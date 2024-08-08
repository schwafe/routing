#include <limits>
#include "channel.hpp"

bool channelInfo::isFull(unsigned char maxTracks)
{
    return channelInfo::usedTracks == maxTracks;
}

unsigned char channelInfo::getUsedTracks()
{
    return usedTracks;
}

void channelInfo::useChannel()
{
    tracks <<= 1;
    tracks[0] = 1;
    usedTracks++;
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
    if (channelID::type == 'x')
    {
        if (y > 0)
        {
            neighbours.emplace(x - 1, y, 'y');
            neighbours.emplace(x, y, 'y');
        }
        if (y < arraySize)
        {
            neighbours.emplace(x - 1, y + 1, 'y');
            neighbours.emplace(x, y + 1, 'y');
        }
        if (x > 1)
        {
            neighbours.emplace(x - 1, y, 'x');
        }
        if (x < arraySize)
        {
            neighbours.emplace(x + 1, y, 'x');
        }
    }
    else
    {
        if (x > 0)
        {
            neighbours.emplace(x, y - 1, 'x');
            neighbours.emplace(x, y, 'x');
        }
        if (x < arraySize)
        {
            neighbours.emplace(x + 1, y - 1, 'x');
            neighbours.emplace(x + 1, y, 'x');
        }
        if (y > 1)
        {
            neighbours.emplace(x, y - 1, 'y');
        }
        if (y < arraySize)
        {
            neighbours.emplace(x, y + 1, 'y');
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

    if (channelID::type == 'x')
    {
        if (y > 0)
        {
            insertIfValid(x - 1, y, 'y', indices, expectedIndex, candidates);
            insertIfValid(x, y, 'y', indices, expectedIndex, candidates);
        }
        if (y < arraySize)
        {
            insertIfValid(x - 1, y + 1, 'y', indices, expectedIndex, candidates);
            insertIfValid(x, y + 1, 'y', indices, expectedIndex, candidates);
        }
    }
    else
    {
        if (x > 0)
        {
            insertIfValid(x, y - 1, 'x', indices, expectedIndex, candidates);
            insertIfValid(x, y, 'x', indices, expectedIndex, candidates);
        }
        if (x < arraySize)
        {
            insertIfValid(x + 1, y - 1, 'x', indices, expectedIndex, candidates);
            insertIfValid(x + 1, y, 'x', indices, expectedIndex, candidates);
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
    if (channelID::type == 'x')
    {
        channelID leftChannel(x - 1, y, 'x');
        channelID rightChannel(x + 1, y, 'x');
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
        channelID lowerChannel(x, y - 1, 'y');
        channelID upperChannel(x, y + 1, 'y');
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
            channelInformation.emplace(channelID(x, y, 'x'), channelInfo());
        }
    }

    for (int x = 0; x <= arraySize; x++)
    {
        for (int y = 1; y <= arraySize; y++)
        {
            channelInformation.emplace(channelID(x, y, 'y'), channelInfo());
        }
    }
    return channelInformation;
}