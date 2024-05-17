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

namespace request {

    static httplib::Headers collectHeaders(const json& input)
    {
        httplib::Headers headers;

        if (helpers::hasField(input, "headers")) {
            for (auto& h : input["headers"].items()) {
                h.key();
                headers.insert({ h.key(), h.value().template get<string>() });
            }
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
        if (!helpers::hasField(input, "headers")) {
            return "application/octet-stream";
        }
        for (auto& p : input["headers"].items()) {
            if (toLower(p.key()) == "content-type") {
                return p.value();
            }
        }
        return "application/octet-stream";
    }

    json get(const json& input)
    {
        json output;

        auto host = input["host"].template get<string>();
        auto path = input["path"].template get<string>();

        httplib::Client cli(host);

        auto res = cli.Get(path, collectHeaders(input));

        if (res.error() == httplib::Error::Success) {
            assembleResponse(res.value(), output);
        } else {
            output["error"] = res.error();
        }

        return output;
    }

    json post(const json& input)
    {
        json output;

        auto host = input["host"].template get<string>();
        auto path = input["path"].template get<string>();
        auto body = base64::from_base64(input["body"].template get<string>());
        auto contentType = inferContentType(input);

        httplib::Client cli(host);

        auto res = cli.Post(path, collectHeaders(input), body.c_str(), body.length(), contentType);

        if (res.error() == httplib::Error::Success) {
            assembleResponse(res.value(), output);
        } else {
            output["error"] = res.error();
        }

        return output;
    }
}

}