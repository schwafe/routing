#include <cassert>
#include <bitset>
#include "channelID.hpp"
#include "../constants.hpp"
#include "../logging.hpp"

unsigned char channelID::getXCoordinate() const
{
    return x;
}

unsigned char channelID::getYCoordinate() const
{
    return y;
}

channelType channelID::getType() const
{
    return type;
}

std::set<channelID> channelID::generateNeighbours(unsigned char arraySize) const
{
    std::set<channelID> neighbours{};
    if (type == channelType::horizontal)
    {
        if (y > 0)
        {
            neighbours.emplace(x - 1, y, channelType::vertical);
            neighbours.emplace(x, y, channelType::vertical);
        }
        if (y < arraySize)
        {
            neighbours.emplace(x - 1, y + 1, channelType::vertical);
            neighbours.emplace(x, y + 1, channelType::vertical);
        }
        if (x > 1)
        {
            neighbours.emplace(x - 1, y, channelType::horizontal);
        }
        if (x < arraySize)
        {
            neighbours.emplace(x + 1, y, channelType::horizontal);
        }
    }
    else
    {
        if (x > 0)
        {
            neighbours.emplace(x, y - 1, channelType::horizontal);
            neighbours.emplace(x, y, channelType::horizontal);
        }
        if (x < arraySize)
        {
            neighbours.emplace(x + 1, y - 1, channelType::horizontal);
            neighbours.emplace(x + 1, y, channelType::horizontal);
        }
        if (y > 1)
        {
            neighbours.emplace(x, y - 1, channelType::vertical);
        }
        if (y < arraySize)
        {
            neighbours.emplace(x, y + 1, channelType::vertical);
        }
    }
    return std::move(neighbours);
}