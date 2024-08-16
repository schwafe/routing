#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <regex>
#include <string>

namespace constants
{
    // TODO change for standalone tests
    const std::string netPrefix = "../test files/net/";
    const std::string placePrefix = "../test files/vpr_placements/";
    const std::string routingPrefix = "../test files/vpr_routings/";

    const std::string netSuffix = ".net";
    const std::string placementSuffix = ".place";
    const std::string routingSuffix = "_my.route";

    const unsigned char success = 0;
    const unsigned char wrongArguments = 1;
    const unsigned char invalidNetFile = 2;
    const unsigned char invalidPlaceFile = 3;

    const unsigned char startingValueChannelWidth = 12;
    const unsigned char maximumChannelWidth = 16;

    const std::regex globalPattern("^\\.global (\\S+)\\s*.*");
    const std::regex inputPattern("^\\.input (\\S+)\\s*.*");
    const std::regex outputPattern("^\\.output (\\S+)\\s*.*");
    const std::regex padPinPattern("^pinlist: (\\S+)\\s*.*");
    const std::regex clbPattern("^\\.clb (\\S+)\\s*.*");
    const std::regex clbPinPattern("^pinlist: (\\S+) (\\S+) (\\S+) (\\S+) (\\S+) (\\S+)\\s*.*");
    const std::regex clbSubPattern("^subblock:.*");

    const std::regex arraySizePattern("(\\d{1,2}) x \\d{1,2}");
    const std::regex commentPattern("^\\s*#.*");
    const std::regex placePattern("^(\\S+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d)\\s");

    const char channelTypeY = 'Y';
    const char channelTypeX = 'X';

    const char blockTypeCLB = 'C';
    const char blockTypeInput = 'I';
    const char blockTypeOutput = 'O';

    const unsigned char inputPinClass = 0;
    const unsigned char outputPinClass = 1;
    const unsigned char clockPinClass = 2;
    const char irrelevantPinClass = -1;
    const unsigned char lowerInputPinNumber = 0;
    const unsigned char leftInputPinNumber = 1;
    const unsigned char rightInputPinNumber = 2;
    const unsigned char upperInputPinNumber = 3;
    const unsigned char outputPinNumber = 4;

    const unsigned char indexZero = 0;
}

#endif