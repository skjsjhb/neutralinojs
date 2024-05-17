#ifndef NEU_ALT_H
#define NEU_ALT_H

#include "lib/json/json.hpp"

using json = nlohmann::json;
using namespace std;

namespace alt {

json vendor(const json& input);

/**
 * Performs HTTP requests. Params are below:
 * host - The host name.
 * path - Path to request.
 * method - HTTP method.
 * headers - HTTP headers.
 * body - Request body.
 *
 * All fields are mandatory. They can be left as empty if unused, but they must exist.
 */
json request(const json& input);
}

#endif // #define NEU_ALT_H
