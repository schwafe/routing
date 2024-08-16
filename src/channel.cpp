#include <limits>
#include <cassert>
#include <bitset>
#include "channel.hpp"
#include "constants.hpp"
#include "logging.hpp"

bool channelInfo::isFull(unsigned char channelwidth) const
{
    return channelwidth == channelInfo::tracksUsed;
}

unsigned char channelInfo::getUsedTracks() const
{
    return tracksUsed;
}

bool channelInfo::isTrackFree(unsigned char track) const
{
    return !occupiedTracks[track];
}

unsigned char channelInfo::useChannel(unsigned char optimalTrack)
{
    unsigned char track = std::numeric_limits<unsigned char>::max();
    if (optimalTrack != std::numeric_limits<unsigned char>::max() && !occupiedTracks[optimalTrack])
    {
        track = optimalTrack;
    }
    else
    {
        unsigned char index = 0;
        do
        {
            if (!occupiedTracks[index])
                track = index;

        } while (track == std::numeric_limits<unsigned char>::max() && ++index < occupiedTracks.size());
    }
    assert(track != std::numeric_limits<unsigned char>::max());
    occupiedTracks[track] = true;
    tracksUsed++;
    return track;
}

channelID::channelID(unsigned char x, unsigned char y, char type)
{
    channelID::x = x;
    channelID::y = y;
    channelID::type = type;
}

channelID::channelID() {}

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

std::set<channelID> channelID::getNeighbours(unsigned char arraySize) const
{
    std::set<channelID> neighbours{};
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

channelID channelID::chooseNeighbour(std::set<channelID> const &validChannels, unsigned char currentTrack, std::map<channelID, channelInfo> const &channelInformation) const
{
    channelID chosenNeighbour{};
    channelID channelA{}, channelB{};
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

        //std::string loggingMessage = "candidates: ";
        for (channelID neighbour : neighbours)
        {
            if (validChannels.contains(neighbour))
            {
                assert(channelInformation.contains(neighbour));
                //loggingMessage += channelIDToString(neighbour) + ' ';
                unsigned char used = channelInformation.find(neighbour)->second.getUsedTracks();
                if (used < lowestAmount)
                {
                    lowestAmount = used;
                    chosenChannel = neighbour;
                }
            }
        }
        //printLogMessage(loggingMessage);

        assert(chosenChannel.isInitialized());
        return chosenChannel;
    }
}

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