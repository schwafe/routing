#include <cassert>
#include "block.hpp"
#include "constants.hpp"

void block::initialise(unsigned char x, unsigned char y, unsigned char subblockNumber)
{
    block::x = x;
    block::y = y;
    block::subblockNumber = subblockNumber;

    std::set<channelID> channels{};
    if (type == blockType::CLB)
    {
        channels.emplace(x, y, channelType::horizontal);
        channels.emplace(x, y, channelType::vertical);
        channels.emplace(x, y - 1, channelType::horizontal);
        channels.emplace(x - 1, y, channelType::vertical);
    }
    else
    {
        if (x == 0)
        {
            channels.emplace(x, y, channelType::vertical);
        }
        else if (y == 0)
        {
            channels.emplace(x, y, channelType::horizontal);
        }
        else if (x > y)
        {
            channels.emplace(x - 1, y, channelType::vertical);
        }
        else
        {
            channels.emplace(x, y - 1, channelType::horizontal);
        }
    }
    block::openPins = channels;
}

blockType block::getType() const
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

std::set<channelID> block::getOpenPins() const
{
    return openPins;
}

unsigned char block::determinePinNumber(channelID const &channel) const
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
            if (channel.getType() == channelType::horizontal)
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

void block::setPinTaken(channelID const &channel)
{
    assert(openPins.contains(channel));
    openPins.erase(channel);
}