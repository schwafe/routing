#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <regex>
#include <string>
#include "channel/channelID.hpp"

namespace constants
{
    const int microsecondsPerMillisecond = 1000;
    const int millisecondsPerSecond = 1000;

    // exit codes
    const unsigned char success = 0;
    const unsigned char wrongArguments = 1;
    const unsigned char invalidNetFile = 2;
    const unsigned char invalidPlaceFile = 3;
    const unsigned char channelWidthTooLow = 4;

    const std::string correctArgumentsMessage = "\nPlease enter either three or four arguments: the filenames of the .net and .place files to be read and the filename of the .route file to be written and optionally a channel width to try the routing for.\n";

    const unsigned char startingValueChannelWidth = 12;
    const unsigned char maximumChannelWidth = 32;
    const unsigned char additionalIterationsForDoublyRelevantChannels = 1;
    /* This determines how much smaller the index of a new track must be, to prefer that result over the best result of the previously used tracks */
    const float ratioNewToOld = 2;
    /* This determines when a failed routing is considered an early fail. If the number of successfully routed nets multiplied by this factor is still
    smaller than the total of nets, it is an early fail. */
    const float ratioEarlyFail = 1.5;

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

    const std::regex numberPattern("^\\d+$");

    const unsigned char outputPinClass = 1;
    const unsigned char clockPinClass = 2;
    const char irrelevantPinClass = -1;
    const unsigned char lowerInputPinNumber = 0;
    const unsigned char leftInputPinNumber = 1;
    const unsigned char upperInputPinNumber = 2;
    const unsigned char rightInputPinNumber = 3;
    const unsigned char outputPinNumber = 4;

    const unsigned char indexZero = 0;
}

#endif