#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>

namespace constants {
    //TODO change for standalone tests
    const std::string netPrefix = "../test files/net/";
    const std::string placementPrefix = "../test files/vpr_placements/";

    const std::string netSuffix = ".net";
    const std::string placementSuffix = ".place";
    const std::string routingSuffix = ".route";

    const unsigned int maxLineLength = 256;

    const unsigned char success = 0;
    const unsigned char wrongArguments = 1;
    const unsigned char readFailure = 2;


    const std::regex globalPattern("^\\.global (\\S+)\\s*.*");
    const std::regex inputPattern("^\\.input (\\S+)\\s*.*");
    const std::regex outputPattern("^\\.output (\\S+)\\s*.*");
    const std::regex padPinPattern("^pinlist: (\\S+)\\s*.*");
    const std::regex clbPattern("^\\.clb (\\S+)\\s*.*");
    const std::regex clbPinPattern("^pinlist: (\\S+) (\\S+) (\\S+) (\\S+) (\\S+) (\\S+)\\s*.*");
    const std::regex clbSubPattern("^subblock:.*");

    const std::regex commentPattern("^\\s*#.*");
    const std::regex placePattern("^(\\S+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d)\\s");
}

#endif