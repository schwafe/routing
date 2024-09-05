#ifndef FINDRESULT_HPP
#define FINDRESULT_HPP

#include "constants.hpp"

struct findResult
{
    unsigned char track{};
    channelID chosenChannel;
    unsigned char indexOfChosenChannel = std::numeric_limits<unsigned char>::max();

public:
    findResult() {};
    findResult(auto t, channelID cC, unsigned char iOCC) : track(t), chosenChannel(cC), indexOfChosenChannel(iOCC) {};
    bool isInitialized() const { return indexOfChosenChannel != std::numeric_limits<unsigned char>::max(); };
};

#endif