#ifndef BLOCK_HPP
#define BLOCK_HPP

#include <set>
#include <limits>
#include "channel.hpp"

class block
{
    char type{}; // C for CLB, P for pad
    unsigned char x{};
    unsigned char y{};
    unsigned char subblockNumber = std::numeric_limits<unsigned char>::max(); // only relevant if type is pad
    std::set<channelID> openChannels{};

public:
    block(char type);
    block(const block &other);
    void initialise(unsigned char x, unsigned char y, unsigned char subblockNumber);

    char getType() const;
    unsigned char getX() const;
    unsigned char getY() const;
    unsigned char getSubblockNumber() const;
    std::set<channelID> getOpenChannels() const;

    unsigned char determinePinNumber(channelID const &channel) const;

    void setChannelTaken(channelID const &channel);
};

#endif
