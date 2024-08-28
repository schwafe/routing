#include <limits>
#include <cassert>
#include "channelInfo.hpp"

bool channelInfo::isFull(unsigned char channelwidth) const
{
    return channelwidth == channelInfo::tracksUsed;
}

unsigned char channelInfo::getTracksUsed() const
{
    return tracksUsed;
}

unsigned char channelInfo::findFreeTrack(unsigned char channelWidth) const
{
    assert(!isFull(channelWidth));
    for (unsigned char track = 0; track < channelWidth; track++)
    {
        if (!occupiedTracks[track])
            return track;
    }
    assert(false);
    return std::numeric_limits<unsigned char>::max();
}

bool channelInfo::isTrackFree(unsigned char track) const
{
    return !occupiedTracks[track];
}

void channelInfo::useChannel(unsigned char track)
{
    occupiedTracks[track] = true;
    tracksUsed++;
}