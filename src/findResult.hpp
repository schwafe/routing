#ifndef FINDRESULT_HPP
#define FINDRESULT_HPP

#include <cassert>
#include <cstdint>
#include "constants.hpp"

struct findResult
{
    unsigned char track{};
    channelID chosenChannel;
    std::int16_t indexOfChosenChannel = std::numeric_limits<int16_t>::max();

public:
    findResult() {};
    findResult(auto t, channelID cC, unsigned char iOCC) : track(t), chosenChannel(cC), indexOfChosenChannel(iOCC) { assert(cC.isInitialised()); };
    bool isInitialized() const { return indexOfChosenChannel != std::numeric_limits<int16_t>::max(); };
};

#endif