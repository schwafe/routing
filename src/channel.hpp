#ifndef CHANNEL_H
#define CHANNEL_H

#include <map>
#include <set>
#include <bitset>
#include "constants.hpp"

class channelInfo
{
    unsigned char tracksUsed{};
    std::bitset<constants::maximumChannelWidth> occupiedTracks{};

public:
    bool isFull(unsigned char channelwidth) const;
    unsigned char getUsedTracks() const;

    unsigned char useChannel(unsigned char optimalTrack);
};

class channelID
{
    unsigned char x{};
    unsigned char y{};
    char type{}; // x or y - horizontal or vertical - channel

public:
    auto operator<=>(channelID const &rhs) const = default;
    channelID(unsigned char x, unsigned char y, char type);
    channelID();

    bool isInitialized() const;
    unsigned char getXCoordinate() const;
    unsigned char getYCoordinate() const;
    char getType() const;
    std::set<channelID> getNeighbours(unsigned char arraySize) const;
    channelID chooseNeighbour(std::set<channelID> const &validChannels, std::map<channelID, channelInfo> const &channelInformation) const;
};

std::map<channelID, channelInfo> generateChannelInformation(unsigned char arraySize);

#endif