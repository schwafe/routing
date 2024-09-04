#include <cassert>
#include <bitset>
#include "channelID.hpp"
#include "../constants.hpp"
#include "../logging.hpp"

bool channelID::isInitialized() const
{
    return type != 0;
}

unsigned char channelID::getXCoordinate() const
{
    return x;
}

unsigned char channelID::getYCoordinate() const
{
    return y;
}

char channelID::getType() const
{
    return type;
}

std::set<channelID> channelID::generateNeighbours(unsigned char arraySize) const
{
    std::set<channelID> neighbours{};
    if (type == constants::channelTypeX)
    {
        if (y > 0)
        {
            neighbours.emplace(x - 1, y, constants::channelTypeY);
            neighbours.emplace(x, y, constants::channelTypeY);
        }
        if (y < arraySize)
        {
            neighbours.emplace(x - 1, y + 1, constants::channelTypeY);
            neighbours.emplace(x, y + 1, constants::channelTypeY);
        }
        if (x > 1)
        {
            neighbours.emplace(x - 1, y, constants::channelTypeX);
        }
        if (x < arraySize)
        {
            neighbours.emplace(x + 1, y, constants::channelTypeX);
        }
    }
    else
    {
        if (x > 0)
        {
            neighbours.emplace(x, y - 1, constants::channelTypeX);
            neighbours.emplace(x, y, constants::channelTypeX);
        }
        if (x < arraySize)
        {
            neighbours.emplace(x + 1, y - 1, constants::channelTypeX);
            neighbours.emplace(x + 1, y, constants::channelTypeX);
        }
        if (y > 1)
        {
            neighbours.emplace(x, y - 1, constants::channelTypeY);
        }
        if (y < arraySize)
        {
            neighbours.emplace(x, y + 1, constants::channelTypeY);
        }
    }
    return std::move(neighbours);
}