#include "api/alt/alt.h"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "helpers.h"
#include "httplib.h"
#include "lib/base64/base64.hpp"
#include "lib/json/json.hpp"
#include <algorithm>
#include <cctype>
#include <string>
#include <thread>

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
}