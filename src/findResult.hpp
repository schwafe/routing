#ifndef FINDRESULT_HPP
#define FINDRESULT_HPP

#include "constants.hpp"

struct findResult
{
    unsigned char track{};
    channelID chosenChannel;
    unsigned char indexOfChosenChannel{};

public:
    findResult() : chosenChannel(constants::uninitializedChannel) {};
    findResult(auto t, channelID cC, unsigned char iOCC) : track(t), chosenChannel(cC), indexOfChosenChannel(iOCC) {};
};

#endif