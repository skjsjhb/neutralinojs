#ifndef NEU_ALT_H
#define NEU_ALT_H

#include "lib/json/json.hpp"

using json = nlohmann::json;
using namespace std;

namespace alt {

json vendor(const json& input);

namespace request {

    /**
     * Performs buffered GET request.
     */
    json get(const json& input);

    /**
     * Performs buffered POST request.
     */
    json post(const json& input);
}

}

#endif // #define NEU_ALT_H
