#include "routing.hpp"

void routeNets(unsigned char &gridSize, std::map<unsigned short, net> &nets, unsigned char &maxTracks)
{
    unsigned short netsRouted = 0;
    std::map<std::string, std::set<channelID>> openPinLocations;
    std::map<unsigned char, std::map<unsigned char, channelInfo>> usedXTracks;
    std::map<unsigned char, std::map<unsigned char, channelInfo>> usedYTracks;
}

void mazeRoute(unsigned char &gridSize, net &net, unsigned char &maxTracks, auto &openPinLocations, auto &usedXTracks, auto &usedYTracks)
{
    std::map<unsigned char, std::set<channelID>> indices;
    std::map<unsigned char, std::set<unsigned char>> usedXChannels;
    std::map<unsigned char, std::set<unsigned char>> usedYChannels;
    unsigned short numberOfPinsReached;

    indices.emplace(0, std::set<channelID>(net.getStartingChannel()));

    while (numberOfPinsReached < net.getPinCount())
    {
        wavePropagation(gridSize, indices, maxTracks, usedXTracks, usedYTracks, usedXChannels, usedYChannels);
    }
}

void addChannelIfValid(char &x, char &y, char &type, unsigned char &maxTracks, auto &usedTracks, auto &usedChannels, auto &nextStep)
{
    if (!usedChannels.find(x)->second.contains(y) && !usedTracks.find(x)->second.find(y)->second.isFull(maxTracks))
        nextStep.emplace(x, y, type);
    TODO check if pin was reached
}

void wavePropagation(unsigned char &gridSize, auto &indices, unsigned char &maxTracks, auto &usedXTracks, auto &usedYTracks, auto &usedXChannels, auto &usedYChannels)
{
    int currentIndex = 0;
    bool pinFound = false;

    do
    {
        std::set<channelID> &nextStep = indices.get(currentIndex + 1);
        for (channelID channel : indices.get(currentIndex))
        {
            if (channel.getType() == 'x')
            {
                if (channel.getYCoordinate() > 0)
                {
                    addChannelIfValid(channel.getXCoordinate() - 1, channel.getYCoordinate(), 'y', maxTracks, usedYTracks, usedYChannels, nextStep);
                    addChannelIfValid(channel.getXCoordinate(), channel.getYCoordinate(), 'y', maxTracks, usedYTracks, usedYChannels, nextStep);
                }
                if (channel.getYCoordinate() < gridSize)
                {
                    addChannelIfValid(channel.getXCoordinate() - 1, channel.getYCoordinate() + 1, 'y', maxTracks, usedYTracks, usedYChannels, nextStep);
                    addChannelIfValid(channel.getXCoordinate(), channel.getYCoordinate() + 1, 'y', maxTracks, usedYTracks, usedYChannels, nextStep);
                }
                if (channel.getXCoordinate() > 0)
                {
                    addChannelIfValid(channel.getXCoordinate() - 1, channel.getYCoordinate(), 'x', maxTracks, usedXTracks, usedXChannels, nextStep);
                }
                if (channel.getXCoordinate() < gridSize)
                {
                    addChannelIfValid(channel.getXCoordinate() + 1, channel.getYCoordinate(), 'x', maxTracks, usedXTracks, usedXChannels, nextStep);
                }
            }
            else
            {
                if (channel.getXCoordinate() > 0)
                {
                    addChannelIfValid(channel.getXCoordinate(), channel.getYCoordinate() - 1, 'x', maxTracks, usedXTracks, usedXChannels, nextStep);
                    addChannelIfValid(channel.getXCoordinate(), channel.getYCoordinate(), 'x', maxTracks, usedXTracks, usedXChannels, nextStep);
                }
                if (channel.getXCoordinate() < gridSize)
                {
                    addChannelIfValid(channel.getXCoordinate() + 1, channel.getYCoordinate() - 1, 'x', maxTracks, usedXTracks, usedXChannels, nextStep);
                    addChannelIfValid(channel.getXCoordinate() + 1, channel.getYCoordinate(), 'x', maxTracks, usedXTracks, usedXChannels, nextStep);
                }
                if (channel.getYCoordinate() > 0)
                {
                    addChannelIfValid(channel.getXCoordinate(), channel.getYCoordinate() - 1, 'y', maxTracks, usedYTracks, usedYChannels, nextStep);
                }
                if (channel.getYCoordinate() < gridSize)
                {
                    addChannelIfValid(channel.getXCoordinate(), channel.getYCoordinate() + 1, 'y', maxTracks, usedYTracks, usedYChannels, nextStep);
                }
            }
        }
        currentIndex++;
    } while (!pinFound);
}