#ifndef BLOCK_HPP
#define BLOCK_HPP

#include <set>
#include "channel.hpp"

class block {
    char type; //c for CLB, p for pad
    unsigned char x;
    unsigned char y;
    std::set<channelID> openChannels;

public:
    char getType();
    std::set<channelID> getOpenChannels();
    void setChannelTaken(channelID channel);
    unsigned char determinePinNumber(channelID channel);
};

#endif