#ifndef CHANNELINFO_H
#define CHANNELINFO_H

#include <bitset>
#include "constants.hpp"

class channelInfo
{
    unsigned char tracksUsed{};
    std::bitset<constants::maximumChannelWidth> occupiedTracks{};

public:
    bool isFull(unsigned char channelwidth) const;
    unsigned char getUsedTracks() const;
    bool isTrackFree(unsigned char track) const;

    unsigned char useChannel(unsigned char optimalTrack);
};

#endif