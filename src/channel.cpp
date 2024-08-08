#include "channel.hpp"

channelID::channelID(unsigned char x, unsigned char y, char type)
{
    channelID::x = x;
    channelID::y = y;
    channelID::type = type;
}

unsigned char channelID::getXCoordinate()
{
    return channelID::x;
}

unsigned char channelID::getYCoordinate()
{
    return channelID::y;
}

char channelID::getType()
{
    return channelID::type;
}

bool channelInfo::isFull(unsigned char maxTracks)
{
    return channelInfo::usedTracks == maxTracks;
}