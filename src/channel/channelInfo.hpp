#ifndef CHANNELINFO_H
#define CHANNELINFO_H

#include <bitset>
#include "../constants.hpp"

class channelInfo
{
    unsigned char tracksUsed{};
    std::bitset<constants::maximumChannelWidth> occupiedTracks{};

public:
    bool isFull(unsigned char channelwidth) const;
    unsigned char getTracksUsed() const;
    unsigned char findFreeTrack(unsigned char channelWidth) const;
    bool isTrackFree(unsigned char track) const;

    void useChannel(unsigned char track);
};

#endif