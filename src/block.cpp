#include "block.hpp"

char block::getType()
{
    return type;
}

std::set<channelID> block::getOpenChannels()
{
    return openChannels;
}

void block::setChannelTaken(channelID channel)
{
    openChannels.erase(channel);
}

unsigned char block::determinePinNumber(channelID channel)
{
    unsigned char number = -1;
    if (channel.getYCoordinate() != y)
    {
        number = 0;
    }
    else
    {
        if (channel.getXCoordinate() != x)
        {
            number = 1;
        }
        else
        {
            if (channel.getType() == 'x')
            {
                number = 2;
            }
            else
            {
                number = 3;
            }
        }
    }
    return number;
}