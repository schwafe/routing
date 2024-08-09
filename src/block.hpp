#ifndef BLOCK_HPP
#define BLOCK_HPP

#include <set>
#include <limits>
#include "channel.hpp"

class block
{
    char type; // c for CLB, p for pad
    unsigned char x;
    unsigned char y;
    unsigned char subblockNumber = std::numeric_limits<char>::max(); // only relevant if type is pad
    std::set<channelID> openChannels;

public:
    block(char type);
    void initialise(unsigned char x, unsigned char y, unsigned char subblockNumber);
    char getType();
    unsigned char getX();
    unsigned char getY();
    unsigned char getSubblockNumber();
    std::set<channelID> getOpenChannels();
    void setChannelTaken(channelID channel);
    unsigned char determinePinNumber(channelID channel);
};

#endif
