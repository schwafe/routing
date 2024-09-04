#include <cassert>
#include <limits>
#include "channel.hpp"
#include "../constants.hpp"
#include "../logging.hpp"

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

    return std::move(channelInformation);
}

bool isChannelFull(channelID const &channel, std::map<channelID, channelInfo> const &channelInformation, unsigned char channelWidth)
{
    assert(channelInformation.contains(channel));
    return channelInformation.find(channel)->second.isFull(channelWidth);
}

bool isChannelTrackFree(channelID const &channel, unsigned char track, std::map<channelID, channelInfo> const &channelInformation)
{
    return channelInformation.find(channel)->second.isTrackFree(track);
}

std::set<unsigned char> getFreeTracks(channelID const &channel, std::map<channelID, channelInfo> const &channelInformation, unsigned char channelWidth)
{
    assert(channelInformation.contains(channel));

    std::set<unsigned char> freeTracks{};
    channelInfo info = channelInformation.find(channel)->second;

    for (unsigned char track = 0; track < channelWidth; track++)
        if (info.isTrackFree(track))
            freeTracks.insert(track);

    return freeTracks;
}

unsigned char findOptimalTrack(channelID const &channel, std::map<channelID, channelInfo> const &channelInformation, unsigned char channelWidth)
{
    assert(channelInformation.contains(channel));
    assert(!channelInformation.find(channel)->second.isFull(channelWidth));
    return channelInformation.find(channel)->second.findFreeTrack(channelWidth);
}

void useChannel(channelID const &channel, std::map<channelID, channelInfo> &channelInformation, unsigned char track)
{
    assert(channelInformation.contains(channel));
    channelInformation.find(channel)->second.useChannel(track);
}

void addIfValid(channelID channel, std::set<std::pair<channelID, unsigned char>> &set, std::set<channelID> const &validChannels,
                std::map<channelID, channelInfo> const &channelInformation)
{
    if (validChannels.contains(channel))
    {
        assert(channelInformation.contains(channel));
        set.emplace(channel, channelInformation.find(channel)->second.getTracksUsed());
    }
}

channelID chooseNeighbouringChannel(channelID channel, unsigned char arraySize, std::set<channelID> const &validChannels,
                                                 std::map<channelID, channelInfo> const &channelInformation)
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
        bool aIsRelevant{}, bIsRelevant{};
        unsigned char usedTracksA{}, usedTracksB{};
        if (aExists)
        {
            channelInfo infA = channelInformation.find(channelA)->second;
            usedTracksA = infA.getTracksUsed();

            aIsRelevant = validChannels.contains(channelA);
        }

        if (bExists)
        {
            channelInfo infB = channelInformation.find(channelB)->second;
            usedTracksB = infB.getTracksUsed();

            bIsRelevant = validChannels.contains(channelB);
        }

        if (!aIsRelevant && bIsRelevant)
            return channelB;
        else if (!bIsRelevant && aIsRelevant)
            return channelA;
        else if (aIsRelevant && bIsRelevant)
        {
            if (usedTracksA <= usedTracksB)
                return channelA;
            else
                return channelB;
        }
    }

    // no neighbour of the same type (horizontal/vertical) was valid, so now neighbours of the other type are searched

    std::set<std::pair<channelID, unsigned char>> neighbours{};
    if (channel.getType() == constants::channelTypeX)
    {
        if (y > 0)
        {
            addIfValid(channelID{x, y, constants::channelTypeY}, neighbours, validChannels, channelInformation);
            addIfValid(channelID{(unsigned char)(x - 1), y, constants::channelTypeY}, neighbours, validChannels, channelInformation);
        }

        if (y < arraySize)
        {
            addIfValid(channelID{(unsigned char)(x - 1), (unsigned char)(y + 1), constants::channelTypeY}, neighbours, validChannels, channelInformation);
            addIfValid(channelID{x, (unsigned char)(y + 1), constants::channelTypeY}, neighbours, validChannels, channelInformation);
        }
    }
    else
    {
        if (x > 0)
        {
            addIfValid(channelID{x, (unsigned char)(y - 1), constants::channelTypeX}, neighbours, validChannels, channelInformation);
            addIfValid(channelID{x, y, constants::channelTypeX}, neighbours, validChannels, channelInformation);
        }

        if (x < arraySize)
        {
            addIfValid(channelID{(unsigned char)(x + 1), (unsigned char)(y - 1), constants::channelTypeX}, neighbours, validChannels, channelInformation);
            addIfValid(channelID{(unsigned char)(x + 1), y, constants::channelTypeX}, neighbours, validChannels, channelInformation);
        }
    }

    assert(!neighbours.empty());

    channelID chosenChannel;
    unsigned char lowestAmount = std::numeric_limits<unsigned char>::max();

    for (std::pair<channelID, unsigned char> neighbourPair : neighbours)
    {
        if (neighbourPair.second < lowestAmount)
        {
            lowestAmount = neighbourPair.second;
            chosenChannel = neighbourPair.first;
        }
    }

    assert(chosenChannel.isInitialized());
    return chosenChannel;
}