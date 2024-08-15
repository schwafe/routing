#include <cassert>
#include "block.hpp"
#include "constants.hpp"

block::block(char type)
{
    block::type = type;
}

block::block(const block &other)
{
    type = other.type;
    x = other.x;
    y = other.y;
    subblockNumber = other.subblockNumber;
    openChannels = std::set<channelID>(other.openChannels);
}

void block::initialise(unsigned char x, unsigned char y, unsigned char subblockNumber)
{
    block::x = x;
    block::y = y;
    block::subblockNumber = subblockNumber;

    std::set<channelID> channels{};
    if (type == constants::blockTypeCLB)
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

char block::getType() const
{
    return type;
}

unsigned char block::getX() const
{
    return x;
}

unsigned char block::getY() const
{
    return y;
}

unsigned char block::getSubblockNumber() const
{
    return subblockNumber;
}

std::set<channelID> block::getOpenChannels() const
{
    return openChannels;
}

unsigned char block::determinePinNumber(channelID channel) const
{
    unsigned char number = -1;
    if (channel.getYCoordinate() != y)
    {
        number = constants::lowerInputPinNumber;
    }
    else
    {
        if (channel.getXCoordinate() != x)
        {
            number = constants::leftInputPinNumber;
        }
        else
        {
            if (channel.getType() == constants::channelTypeX)
            {
                number = constants::upperInputPinNumber;
            }
            else
            {
                number = constants::rightInputPinNumber;
            }
        }
    }
    return number;
}

void block::setChannelTaken(channelID channel)
{
    assert(openChannels.contains(channel));
    openChannels.erase(channel);
}