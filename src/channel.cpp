#include <limits>
#include <cassert>
#include <iostream>
#include "channel.hpp"
#include "constants.hpp"
#include "logging.hpp"

bool channelInfo::isFull(unsigned char channelwidth)
{
    return channelInfo::usedTracks == channelwidth;
}

unsigned char channelInfo::getUsedTracks()
{
    return usedTracks;
}

unsigned char channelInfo::useChannel()
{
    return usedTracks++;
}

channelID::channelID(unsigned char x, unsigned char y, char type)
{
    channelID::x = x;
    channelID::y = y;
    channelID::type = type;
}

channelID::channelID() {}

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

channelID channelID::chooseNeighbour(const std::set<channelID> &validChannels, std::map<channelID, channelInfo> &channelInformation)
{
    channelID chosenNeighbour{};
    channelID channelA, channelB;
    if (channelID::type == constants::channelTypeX)
    {
        channelA = channelID(x - 1, y, constants::channelTypeX);
        channelB = channelID(x + 1, y, constants::channelTypeX);
    }
    else
    {
        channelA = channelID(x, y - 1, constants::channelTypeY);
        channelB = channelID(x, y + 1, constants::channelTypeY);
    }

    if (!validChannels.contains(channelA) && validChannels.contains(channelB))
        return channelB;
    else if (!validChannels.contains(channelB) && validChannels.contains(channelA))
        return channelA;
    else if (validChannels.contains(channelB) && validChannels.contains(channelA))
    {
        if (channelInformation.find(channelA)->second.getUsedTracks() <= channelInformation.find(channelB)->second.getUsedTracks())
            return channelA;
        else
            return channelB;
    }
    else
    {
        // no same type neighbour was valid, so now other type neighbours are searched

        std::set<channelID> neighbours{};
        if (type == constants::channelTypeX)
        {
            neighbours.emplace(x - 1, y, constants::channelTypeY);
            neighbours.emplace(x, y, constants::channelTypeY);
            neighbours.emplace(x - 1, y + 1, constants::channelTypeY);
            neighbours.emplace(x, y + 1, constants::channelTypeY);
        }
        else
        {
            neighbours.emplace(x, y - 1, constants::channelTypeX);
            neighbours.emplace(x, y, constants::channelTypeX);
            neighbours.emplace(x + 1, y - 1, constants::channelTypeX);
            neighbours.emplace(x + 1, y, constants::channelTypeX);
        }

        unsigned char lowestAmount = std::numeric_limits<unsigned char>::max();
        channelID chosenChannel{};

        std::cout << "candidates: ";
        for (channelID neighbour : neighbours)
        {
            if (validChannels.contains(neighbour))
            {
                std::cout << channelIDToString(neighbour) << ' ';
                assert(channelInformation.contains(neighbour));
                unsigned char used = channelInformation.find(neighbour)->second.getUsedTracks();
                if (used < lowestAmount)
                {
                    lowestAmount = used;
                    chosenChannel = neighbour;
                }
            }
        }
        std::cout << std::endl;

        assert(chosenChannel.getType() != 0);
        return chosenChannel;
    }
}

channelID chooseSameTypeNeighbour(unsigned char arraySize, std::map<channelID, unsigned char> &indices, unsigned char expectedIndex, std::map<channelID, channelInfo> &channelInformation, channelID &channelA, channelID &channelB, unsigned char coordinate)
{
    if (coordinate == arraySize || !indices.contains(channelB))
    {
        if (indices.contains(channelA) && indices.find(channelA)->second == expectedIndex)
            return channelA;
        else
            return channelID{};
    }
    else if (coordinate == 0 || !indices.contains(channelA))
    {
        if (indices.contains(channelB) && indices.find(channelB)->second == expectedIndex)
            return channelB;
        else
            return channelID{};
    }
    else
    {
        unsigned char indexA = indices.find(channelA)->second;
        unsigned char indexB = indices.find(channelB)->second;

        if (indexA > expectedIndex && indexB > expectedIndex)
            return channelID{};
        else if (indexB > expectedIndex)
            return channelA;
        else if (indexA > expectedIndex)
            return channelB;
        else if (channelInformation.find(channelA)->second.getUsedTracks() <= channelInformation.find(channelB)->second.getUsedTracks())
            return channelA;
        else
            return channelB;
    }
}

void insertIfValid(unsigned char x, unsigned char y, char type, std::map<channelID, unsigned char> &indices, unsigned char expectedIndex, std::set<channelID> &candidates)
{
    channelID channel(x, y, type);
    if (indices.contains(channel) && indices.find(channel)->second == expectedIndex)
        candidates.insert(channel);
}

channelID channelID::chooseNeighbour_old(unsigned char arraySize, std::map<channelID, unsigned char> &indices, unsigned char expectedIndex, std::map<channelID, channelInfo> &channelInformation)
{
    channelID chosenNeighbour{};
    if (channelID::type == constants::channelTypeX)
    {
        channelID leftChannel(x - 1, y, constants::channelTypeX);
        channelID rightChannel(x + 1, y, constants::channelTypeX);
        chosenNeighbour = chooseSameTypeNeighbour(arraySize, indices, expectedIndex, channelInformation, leftChannel, rightChannel, x);
    }
    else
    {
        channelID lowerChannel(x, y - 1, constants::channelTypeY);
        channelID upperChannel(x, y + 1, constants::channelTypeY);
        chosenNeighbour = chooseSameTypeNeighbour(arraySize, indices, expectedIndex, channelInformation, lowerChannel, upperChannel, y);
    }

    if (chosenNeighbour.type == 0)
    {
        std::set<channelID> candidates;

        if (type == constants::channelTypeX)
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

        assert(candidates.size() != 0);

        unsigned char lowestAmount = std::numeric_limits<unsigned char>::max();
        for (channelID channel : candidates)
        {
            assert(channelInformation.contains(channel));
            unsigned char used = channelInformation.find(channel)->second.getUsedTracks();
            if (used < lowestAmount)
                lowestAmount = used;
        }

        for (channelID channel : candidates)
        {
            std::cout << "candidate: " << channelIDToString(channel) << std::endl;
            if (channelInformation.find(channel)->second.getUsedTracks() == lowestAmount)
            {
                chosenNeighbour = channel;
                break;
            }
        }
    }

    return chosenNeighbour;
}

std::map<channelID, channelInfo> generateChannelInformation(unsigned char arraySize)
{
    std::map<channelID, channelInfo> channelInformation;
    for (int x = 1; x <= arraySize; x++)
    {
        for (int y = 0; y <= arraySize; y++)
        {
            channelInformation.emplace(channelID(x, y, constants::channelTypeX), channelInfo{});
        }
    }

    for (int x = 0; x <= arraySize; x++)
    {
        for (int y = 1; y <= arraySize; y++)
        {
            channelInformation.emplace(channelID(x, y, constants::channelTypeY), channelInfo{});
        }
    }

    return channelInformation;
}