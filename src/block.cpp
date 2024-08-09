#include "block.hpp"
#include "constants.hpp"

block::block(char type)
{
    block::type = type;
}

void block::initialise(unsigned char x, unsigned char y, unsigned char subblockNumber)
{
    block::x = x;
    block::y = y;
    block::subblockNumber = subblockNumber;

    std::set<channelID> channels;
    if (type == 'c')
    {
        channels.emplace(x, y, constants::channelTypeX);
        channels.emplace(x, y, constants::channelTypeY);
        channels.emplace(x, y - 1, constants::channelTypeX);
        channels.emplace(x - 1, y, constants::channelTypeY);
    }
    else
    {
        if (x == 0)
        {
            channels.emplace(x, y, constants::channelTypeY);
        }
        else if (y == 0)
        {
            channels.emplace(x, y, constants::channelTypeX);
        }
        else if (x > y)
        {
            channels.emplace(x - 1, y, constants::channelTypeY);
        }
        else
        {
            channels.emplace(x, y - 1, constants::channelTypeX);
        }
    }
    block::openChannels = channels;
}

char block::getType()
{
    return type;
}

unsigned char block::getX()
{
    return x;
}

unsigned char block::getY()
{
    return y;
}

unsigned char block::getSubblockNumber()
{
    return subblockNumber;
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
            if (channel.getType() == constants::channelTypeX)
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