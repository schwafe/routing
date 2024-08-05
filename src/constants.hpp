#include <string>

namespace constants {
    //TODO change for standalone tests
    const std::string placementPrefix = "../test files/vpr_placements/";

    const unsigned int maxLineLength = 256;

    const unsigned char success = 0;
    const unsigned char readFailure = 1;

    const std::regex commentPattern("^\\s*#.*");
    const std::regex inputPattern("^(\\S+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d)\\s");
}

