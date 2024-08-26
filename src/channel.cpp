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

channelID chooseNeighbouringChannel(channelID channel, std::set<channelID> const &validChannels, unsigned char currentTrack, std::map<channelID, channelInfo> const &channelInformation)
{
    unsigned char x = channel.getXCoordinate();
    unsigned char y = channel.getYCoordinate();

    channelID chosenNeighbour{};
    channelID channelA{}, channelB{};
    if (channel.getType() == constants::channelTypeX)
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
    else
    {
        // no same type neighbour was valid, so now other type neighbours are searched

        std::set<channelID> neighbours{};
        if (channel.getType() == constants::channelTypeX)
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

        std::set<channelID> validChannelsWithFreeCurrentTrack{};

        for (channelID neighbour : neighbours)
        {
            if (validChannels.contains(neighbour) && channelInformation.find(neighbour)->second.isTrackFree(currentTrack))
                validChannelsWithFreeCurrentTrack.insert(neighbour);
        }

        if (validChannelsWithFreeCurrentTrack.size() >= 1)
            neighbours = validChannelsWithFreeCurrentTrack;

        unsigned char lowestAmount = std::numeric_limits<unsigned char>::max();
        channelID chosenChannel{};

        for (channelID neighbour : neighbours)
        {
            if (validChannels.contains(neighbour))
            {
                assert(channelInformation.contains(neighbour));
                unsigned char used = channelInformation.find(neighbour)->second.getUsedTracks();
                if (used < lowestAmount)
                {
                    lowestAmount = used;
                    chosenChannel = neighbour;
                }
            }
        }

        assert(chosenChannel.isInitialized());
        return chosenChannel;
    }
}