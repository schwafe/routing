#ifndef CHANNEL_H
#define CHANNEL_H

#include <map>
#include <set>
#include <bitset>

class channelInfo
{
    unsigned char usedTracks{};
    std::bitset<2> openTracks;

public:
    bool isFull(unsigned char channelwidth) const;
    unsigned char getUsedTracks() const;

    unsigned char useChannel(const unsigned char &optimalTrack);
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

    unsigned char getXCoordinate() const;
    unsigned char getYCoordinate() const;
    char getType() const;
    std::set<channelID> getNeighbours(unsigned char arraySize) const;
    channelID chooseNeighbour(const std::set<channelID> &validChannels, std::map<channelID, channelInfo> &channelInformation) const;
};

std::map<channelID, channelInfo> generateChannelInformation(unsigned char arraySize);

#endif