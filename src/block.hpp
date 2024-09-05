#ifndef BLOCK_H
#define BLOCK_H

#include <set>
#include <limits>
#include "channel/channel.hpp"

enum blockType
{
    CLB,
    input,
    output
};

class block
{
    blockType type;
    unsigned char x{};
    unsigned char y{};
    unsigned char subblockNumber{}; // only relevant if type is input or output
    std::set<channelID> openPins;

public:
    block(blockType type) : type(type) {};
    void initialise(unsigned char x, unsigned char y, unsigned char subblockNumber);

    blockType getType() const;
    unsigned char getX() const;
    unsigned char getY() const;
    unsigned char getSubblockNumber() const;
    std::set<channelID> getOpenPins() const;

    unsigned char determinePinNumber(channelID const &channel) const;

    void setPinTaken(channelID const &channel);
};

#endif
