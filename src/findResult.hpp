#ifndef FINDRESULT_HPP
#define FINDRESULT_HPP

#include "constants.hpp"

struct findResult
{
    unsigned char track;
    std::map<channelID, unsigned char> channelToIndex;
    std::map<unsigned char, std::set<channelID>> indexToChannels;
    channelID chosenChannel;
    unsigned char indexOfChosenChannel;

public:
    findResult() : chosenChannel(constants::uninitializedChannel) {};
    findResult(auto t, channelID cC, unsigned char iOCC) : track(t), chosenChannel(cC), indexOfChosenChannel(iOCC) {};
    findResult(auto t, auto cTI, auto iTC, channelID cC, unsigned char iOCC) : track(t), channelToIndex(cTI), indexToChannels(iTC), chosenChannel(cC), indexOfChosenChannel(iOCC) {};
};

#endif