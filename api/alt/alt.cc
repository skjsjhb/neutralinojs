#include "api/alt/alt.h"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "helpers.h"
#include "httplib.h"
#include "lib/base64/base64.hpp"
#include "lib/json/json.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

using namespace std;
using json = nlohmann::json;

static httplib::Server altServer;

namespace alt {

json vendor(const json& input)
{
    json output;
    output["returnValue"] = "Alt Server";
    output["success"] = true;
    return output;
}

static httplib::Headers collectHeaders(const json& input)
{
    httplib::Headers headers;

    for (auto& h : input["headers"].items()) {
        h.key();
        headers.insert({ h.key(), h.value().template get<string>() });
    }

    return headers;
}

static void assembleResponse(const httplib::Response& response, json& output)
{
    output["success"] = true;
    output["returnValue"] = {
        { "statusCode", response.status },
        { "headers", {} }, // Leave it empty for now
        { "body", base64::to_base64(response.body) }
    };

    for (auto& p : response.headers) {
        output["returnValue"]["headers"][p.first] = p.second;
    }
}

static string toLower(string src)
{
    std::transform(src.begin(), src.end(), src.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return src;
}

static string inferContentType(const json& input)
{
    for (auto& p : input["headers"].items()) {
        if (toLower(p.key()) == "content-type") {
            return p.value();
        }
    }
    return "application/octet-stream";
}

json request(const json& input)
{
    json output;

    auto host = input["host"].template get<string>();
    auto path = input["path"].template get<string>();
    auto method = input["method"].template get<string>();
    auto bodyBase = input["body"].template get<string>();

    auto body = base64::from_base64(bodyBase);
    auto contentType = inferContentType(input);
    auto headers = collectHeaders(input);

    httplib::Client cli(host);
    httplib::Result res;

    if (method == "GET") {
        res = cli.Get(path, headers);
    } else if (method == "POST") {
        res = cli.Post(path, headers, body, contentType);
    } else if (method == "HEAD") {
        res = cli.Head(path, headers);
    } else if (method == "PUT") {
        res = cli.Put(path, headers, body, contentType);
    } else if (method == "PATCH") {
        res = cli.Patch(path, headers, body, contentType);
    } else if (method == "DELETE") {
        res = cli.Delete(path, headers, body, contentType);
    } else if (method == "OPTIONS") {
        res = cli.Options(path, headers);
    }

    if (res.error() == httplib::Error::Success) {
        assembleResponse(res.value(), output);
    } else {
        output["error"] = res.error();
    }

    return output;
}

json dl(const json& input)
{
    json output;

    auto host = input["host"].template get<string>();
    auto path = input["path"].template get<string>();
    auto file = input["file"].template get<string>();

    auto headers = collectHeaders(input);

    httplib::Client cli(host);

    ofstream* f = nullptr;

    bool err = false;
    std::string msg;

    auto contentHandler = [&](const char* data, size_t data_length) {
        try {
            if (f == nullptr) {
                err = true;
                return false;
            }
            *f << std::string(data, data_length);
            if (f->fail()) {
                err = true;
                msg = "Failed to write file";
                return false;
            }
            return true;
        } catch (std::exception& e) {
            err = true;
            msg = e.what();
            return false;
        }
    };

    int status = 0;

    auto responseHandler = [&](httplib::Response res) {
        if (res.status >= 400) {
            err = true;
            msg = "Invalid status: " + std::to_string(res.status);
            return false;
        } else {
            status = res.status;
            f = new ofstream(file, std::ios::out | std::ios::binary | std::ios::trunc);
            return true;
        }
    };

    cli.Get(path, headers, responseHandler, contentHandler);

    if (f != nullptr) {
        f->close();
        delete f;
    }

    if (err) {
        output["error"] = msg;
    } else {
        output["success"] = true;
        output["returnValue"] = status;
    }

    return output;
}

json dlMulti(const json& input)
{
    json output;
    output["returnValue"] = {};
    output["success"] = true;

    std::vector<std::thread> threads;

    auto len = input.size();

    string* errors = new string[len];
    int* status = new int[len];

    int i = 0;
    for (auto& req : input) {
        std::thread executor([=]() {
            auto result = dl(req);
            if (helpers::hasField(result, "error")) {
                errors[i] = result["error"].template get<string>();
                status[i] = 0;
            } else {
                errors[i] = "";
                status[i] = result["returnValue"].template get<int>();
            }
        });
        threads.push_back(std::move(executor));
        i++;
    }
    for (auto& t : threads) {
        t.join();
    }

    output["returnValue"] = {
        { "errors", {} },
        { "statusCodes", {} }
    };

    for (int k = 0; k < i; k++) {
        output["returnValue"]["errors"].push_back(errors[k]);
        output["returnValue"]["statusCodes"].push_back(status[k]);
    }

    delete[] errors;
    delete[] status;

    return output;
}
}