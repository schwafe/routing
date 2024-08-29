#ifndef BLOCK_H
#define BLOCK_H

#include <set>
#include <limits>
#include "channel/channel.hpp"

class block
{
    char type; // C for CLB, P for pad
    unsigned char x{};
    unsigned char y{};
    unsigned char subblockNumber{}; // only relevant if type is pad
    std::set<channelID> openPins;

public:
    block(char type) : type(type) {};
    void initialise(unsigned char x, unsigned char y, unsigned char subblockNumber);

    char getType() const;
    unsigned char getX() const;
    unsigned char getY() const;
    unsigned char getSubblockNumber() const;
    std::set<channelID> getOpenPins() const;

    unsigned char determinePinNumber(channelID const &channel) const;

    void setPinTaken(channelID const &channel);
};

#endif
