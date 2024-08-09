#ifndef CHANNEL_H
#define CHANNEL_H

#include <map>
#include <set>
#include <bitset>

class channelInfo
{
    unsigned char usedTracks;
    std::bitset<2> tracks;

public:
    bool isFull(unsigned char maxTracks);
    unsigned char getUsedTracks();
    unsigned char useChannel();
};

class channelID
{
    unsigned char x;
    unsigned char y;
    char type; // x or y - horizontal or vertical - channel

public:
    auto operator<=>(channelID const &rhs) const = default;
    channelID(unsigned char x, unsigned char y, char type);
    channelID();
    unsigned char getXCoordinate();
    unsigned char getYCoordinate();
    char getType();
    std::set<channelID> getNeighbours(unsigned char arraySize);
    channelID chooseNeighbour(unsigned char arraySize, std::map<channelID, unsigned char> &indices, unsigned char expectedIndex, std::map<channelID, channelInfo> &channelInformation);
    channelID chooseSameTypeNeighbours(unsigned char arraySize, std::map<channelID, unsigned char> &indices, unsigned char expectedIndex, std::map<channelID, channelInfo> &channelInformation);
    channelID chooseOtherTypeNeighbours(unsigned char arraySize, std::map<channelID, unsigned char> &indices, unsigned char expectedIndex, std::map<channelID, channelInfo> &channelInformation);
};

std::map<channelID, channelInfo> generateChannelInformation(unsigned char arraySize);

#endif