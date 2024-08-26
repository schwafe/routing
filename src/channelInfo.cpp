#include <limits>
#include <cassert>
#include "channelInfo.hpp"

bool channelInfo::isFull(unsigned char channelwidth) const
{
    return channelwidth == channelInfo::tracksUsed;
}

unsigned char channelInfo::getUsedTracks() const
{
    return tracksUsed;
}

bool channelInfo::isTrackFree(unsigned char track) const
{
    return !occupiedTracks[track];
}

unsigned char channelInfo::useChannel(unsigned char optimalTrack)
{
    unsigned char track = std::numeric_limits<unsigned char>::max();
    if (optimalTrack != std::numeric_limits<unsigned char>::max() && !occupiedTracks[optimalTrack])
    {
        track = optimalTrack;
    }
    else
    {
        unsigned char index = 0;
        do
        {
            if (!occupiedTracks[index])
                track = index;

        } while (track == std::numeric_limits<unsigned char>::max() && ++index < occupiedTracks.size());
    }
    assert(track != std::numeric_limits<unsigned char>::max());
    occupiedTracks[track] = true;
    tracksUsed++;
    return track;
}