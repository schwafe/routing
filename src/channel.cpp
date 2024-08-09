#include <limits>
#include "channel.hpp"
#include "constants.hpp"

bool channelInfo::isFull(unsigned char maxTracks)
{
    return channelInfo::usedTracks == maxTracks;
}

unsigned char channelInfo::getUsedTracks()
{
    return usedTracks;
}

unsigned char channelInfo::useChannel()
{
    return ++usedTracks;
}

channelID::channelID(unsigned char x, unsigned char y, char type)
{
    channelID::x = x;
    channelID::y = y;
    channelID::type = type;
}

channelID::channelID()
{
    x = 0;
    y = 0;
    type = 0;
}

unsigned char channelID::getXCoordinate()
{
    return x;
}

unsigned char channelID::getYCoordinate()
{
    return y;
}

char channelID::getType()
{
    return type;
}

std::set<channelID> channelID::getNeighbours(unsigned char arraySize)
{
    std::set<channelID> neighbours;
    if (channelID::type == constants::channelTypeX)
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
    return neighbours;
}

void insertIfValid(unsigned char x, unsigned char y, char type, std::map<channelID, unsigned char> &indices, unsigned char expectedIndex, std::set<channelID> &candidates)
{
    channelID channel(x, y, type);
    if (indices.find(channel)->second == expectedIndex)
        candidates.insert(channel);
}

channelID channelID::chooseOtherTypeNeighbours(unsigned char arraySize, std::map<channelID, unsigned char> &indices, unsigned char expectedIndex, std::map<channelID, channelInfo> &channelInformation)
{
    std::set<channelID> candidates;

    if (channelID::type == constants::channelTypeX)
    {
        if (y > 0)
        {
            insertIfValid(x - 1, y, constants::channelTypeY, indices, expectedIndex, candidates);
            insertIfValid(x, y, constants::channelTypeY, indices, expectedIndex, candidates);
        }
        if (y < arraySize)
        {
            insertIfValid(x - 1, y + 1, constants::channelTypeY, indices, expectedIndex, candidates);
            insertIfValid(x, y + 1, constants::channelTypeY, indices, expectedIndex, candidates);
        }
    }
    else
    {
        if (x > 0)
        {
            insertIfValid(x, y - 1, constants::channelTypeX, indices, expectedIndex, candidates);
            insertIfValid(x, y, constants::channelTypeX, indices, expectedIndex, candidates);
        }
        if (x < arraySize)
        {
            insertIfValid(x + 1, y - 1, constants::channelTypeX, indices, expectedIndex, candidates);
            insertIfValid(x + 1, y, constants::channelTypeX, indices, expectedIndex, candidates);
        }
    }

    unsigned char lowestAmount = std::numeric_limits<char>::max();
    for (channelID channel : candidates)
    {
        unsigned char used = channelInformation.find(channel)->second.getUsedTracks();
        if (used < lowestAmount)
            lowestAmount = used;
    }

    for (channelID channel : candidates)
    {
        if (channelInformation.find(channel)->second.getUsedTracks() == lowestAmount)
            return channel;
    }

    return channelID();
}

channelID channelID::chooseSameTypeNeighbours(unsigned char arraySize, std::map<channelID, unsigned char> &indices, unsigned char expectedIndex, std::map<channelID, channelInfo> &channelInformation)
{
    if (channelID::type == constants::channelTypeX)
    {
        channelID leftChannel(x - 1, y, constants::channelTypeX);
        channelID rightChannel(x + 1, y, constants::channelTypeX);
        if (x == arraySize || indices.find(rightChannel)->second > expectedIndex)
        {
            return leftChannel;
        }
        else if (x == 0 || indices.find(leftChannel)->second > expectedIndex)
        {
            return rightChannel;
        }
        else
        {
            if (channelInformation.find(leftChannel)->second.getUsedTracks() <= channelInformation.find(rightChannel)->second.getUsedTracks())
            {
                return leftChannel;
            }
            else
            {
                return rightChannel;
            }
        }
    }
    else
    {
        channelID lowerChannel(x, y - 1, constants::channelTypeY);
        channelID upperChannel(x, y + 1, constants::channelTypeY);
        if (y == arraySize || indices.find(upperChannel)->second > expectedIndex)
        {
            return lowerChannel;
        }
        else if (y == 0 || indices.find(lowerChannel)->second > expectedIndex)
        {
            return upperChannel;
        }
        else
        {
            if (channelInformation.find(lowerChannel)->second.getUsedTracks() <= channelInformation.find(upperChannel)->second.getUsedTracks())
            {
                return lowerChannel;
            }
            else
            {
                return upperChannel;
            }
        }
    }
    return channelID();
}

channelID channelID::chooseNeighbour(unsigned char arraySize, std::map<channelID, unsigned char> &indices, unsigned char expectedIndex, std::map<channelID, channelInfo> &channelInformation)
{
    channelID chosenNeighbour = channelID::chooseSameTypeNeighbours(arraySize, indices, expectedIndex, channelInformation);
    if (chosenNeighbour.type == 0)
        chosenNeighbour = channelID::chooseOtherTypeNeighbours(arraySize, indices, expectedIndex, channelInformation);

    return chosenNeighbour;
}

std::map<channelID, channelInfo> generateChannelInformation(unsigned char arraySize)
{
    std::map<channelID, channelInfo> channelInformation;
    for (int x = 1; x <= arraySize; x++)
    {
        for (int y = 0; y <= arraySize; y++)
        {
            channelInformation.emplace(channelID(x, y, constants::channelTypeX), channelInfo());
        }
    }

    for (int x = 0; x <= arraySize; x++)
    {
        for (int y = 1; y <= arraySize; y++)
        {
            channelInformation.emplace(channelID(x, y, constants::channelTypeY), channelInfo());
        }
    }
    return channelInformation;
}