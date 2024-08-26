#include <cassert>
#include <limits>
#include "channel.hpp"
#include "constants.hpp"

std::map<channelID, channelInfo> generateChannelInformation(unsigned char arraySize)
{
    std::map<channelID, channelInfo> channelInformation{};
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

bool isChannelFull(channelID const &channel, std::map<channelID, channelInfo> const &channelInformation, unsigned char channelWidth)
{
    assert(channelInformation.contains(channel));
    return channelInformation.find(channel)->second.isFull(channelWidth);
}

unsigned char useChannel(channelID const &channel, std::map<channelID, channelInfo> &channelInformation, unsigned char optimalTrack)
{
    assert(channelInformation.contains(channel));
    return channelInformation.find(channel)->second.useChannel(optimalTrack);
}

void addIfValid(channelID channel, std::set<channelID> &set, std::set<channelID> const &validChannels)
{
    if (validChannels.contains(channel))
        set.insert(channel);
}

channelID chooseNeighbouringChannel(channelID channel, unsigned char arraySize, std::set<channelID> const &validChannels, unsigned char currentTrack, std::map<channelID, channelInfo> const &channelInformation)
{
    unsigned char x = channel.getXCoordinate();
    unsigned char y = channel.getYCoordinate();

    channelID channelA{}, channelB{};
    bool aExists{}, bExists{};
    if (channel.getType() == constants::channelTypeX)
    {
        aExists = x > 1;
        channelA = channelID(x - 1, y, constants::channelTypeX);
        bExists = x < arraySize;
        channelB = channelID(x + 1, y, constants::channelTypeX);
    }
    else
    {
        aExists = y > 1;
        channelA = channelID(x, y - 1, constants::channelTypeY);
        bExists = y < arraySize;
        channelB = channelID(x, y + 1, constants::channelTypeY);
    }

    if (aExists || bExists)
    {
        bool containsA = aExists && validChannels.contains(channelA);
        bool containsB = bExists && validChannels.contains(channelB);
        if (!containsA && containsB)
            return channelB;
        else if (!containsB && containsA)
            return channelA;
        else if (containsA && containsB)
        {
            channelInfo infA = channelInformation.find(channelA)->second;
            channelInfo infB = channelInformation.find(channelB)->second;

            if (!infA.isTrackFree(currentTrack) && infB.isTrackFree(currentTrack))
                return channelB;
            else if (!infB.isTrackFree(currentTrack) && infA.isTrackFree(currentTrack))
                return channelA;
            else if (infA.getUsedTracks() <= infB.getUsedTracks())
                return channelA;
            else
                return channelB;
        }
    }

    // no neighbour of the same type (horizontal/vertical) was valid, so now neighbours of the other type are searched

    std::set<channelID> neighbours{};
    if (channel.getType() == constants::channelTypeX)
    {
        if (y > 0)
        {
            addIfValid(channelID{x, y, constants::channelTypeY}, neighbours, validChannels);
            addIfValid(channelID{(unsigned char)(x - 1), y, constants::channelTypeY}, neighbours, validChannels);
        }

        if (y < arraySize)
        {
            addIfValid(channelID{(unsigned char)(x - 1), (unsigned char)(y + 1), constants::channelTypeY}, neighbours, validChannels);
            addIfValid(channelID{x, (unsigned char)(y + 1), constants::channelTypeY}, neighbours, validChannels);
        }
    }
    else
    {
        if (x > 0)
        {
            addIfValid(channelID{x, (unsigned char)(y - 1), constants::channelTypeX}, neighbours, validChannels);
            addIfValid(channelID{x, y, constants::channelTypeX}, neighbours, validChannels);
        }

        if (x < arraySize)
        {
            addIfValid(channelID{(unsigned char)(x + 1), (unsigned char)(y - 1), constants::channelTypeX}, neighbours, validChannels);
            addIfValid(channelID{(unsigned char)(x + 1), y, constants::channelTypeX}, neighbours, validChannels);
        }
    }

    assert(!neighbours.empty());

    channelID chosenChannel;
    unsigned char lowestAmount = std::numeric_limits<unsigned char>::max();
    unsigned char lowestAmountWithFreeCurrentTrack = std::numeric_limits<unsigned char>::max();
    std::set<channelID> channelsWithFreeCurrentTrack{};

    for (channelID neighbour : neighbours)
    {
        assert(channelInformation.contains(neighbour));
        channelInfo neighboursInfo = channelInformation.find(neighbour)->second;
        unsigned char usedTracks = neighboursInfo.getUsedTracks();
        if (neighboursInfo.isTrackFree(currentTrack))
        {
            if (usedTracks < lowestAmountWithFreeCurrentTrack)
            {
                lowestAmountWithFreeCurrentTrack = usedTracks;
                chosenChannel = neighbour;
            }
        }
        else
        {
            if (lowestAmountWithFreeCurrentTrack == std::numeric_limits<unsigned char>::max() && usedTracks < lowestAmount)
            {
                lowestAmount = usedTracks;
                chosenChannel = neighbour;
            }
        }
    }

    assert(chosenChannel.isInitialized());
    return chosenChannel;
}