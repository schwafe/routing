#include "net.hpp"

#include <cassert>
#include <iostream>

net::net(std::string name)
{
    net::name = name;
}

net::net(const net &other)
{
    name = other.name;
    sourceBlockName = other.sourceBlockName;
    sourceChannel = other.sourceChannel;
    sinkBlockNames = other.sinkBlockNames;
}

std::string net::getName() const
{
    return name;
}

channelID net::getSourceChannel() const
{
    return sourceChannel;
}

std::string net::getSourceBlockName() const
{
    return sourceBlockName;
}

unsigned short net::getSinkBlockCount() const
{
    return sinkBlockNames.size();
}

bool net::usedChannel(const channelID &channel) const
{
    return usedTracks.contains(channel);
}

std::set<std::string> net::getSinkBlockNames() const
{
    return sinkBlockNames;
}

std::vector<std::pair<std::string, std::vector<std::pair<channelID, unsigned char>>>> net::getConnectionsByRoutingOrder() const
{
    return connectionsByRoutingOrder;
}

unsigned char net::chooseUsedTrack(const channelID &channel, const unsigned char &optimalTrack) const
{
    assert(usedTracks.contains(channel));
    auto range = usedTracks.equal_range(channel);

    for (auto it = range.first; it != range.second; it++)
    {
        if (it->second == optimalTrack)
            return optimalTrack;
    }

    return usedTracks.find(channel)->second;
}

bool net::allPinsConnected() const
{
    return sinkBlockNames.size() == connectionsByRoutingOrder.size();
}

void net::setSourceChannel(channelID sourceChannel)
{
    net::sourceChannel = sourceChannel;
}

void net::setSourceBlockName(std::string sourceBlockName)
{
    net::sourceBlockName = sourceBlockName;
}

void net::addSinkBlock(std::string sinkBlockName)
{
    assert(!sinkBlockNames.contains(sinkBlockName));
    sinkBlockNames.emplace(sinkBlockName);
}

void net::setUsedTrack(const channelID &channel, const unsigned char &track)
{
    usedTracks.emplace(channel, track);
}

void net::setConnection(std::string sinkBlockName, std::vector<std::pair<channelID, unsigned char>> connectionToSink)
{
    assert(sinkBlockNames.contains(sinkBlockName));
    connectionsByRoutingOrder.emplace_back(sinkBlockName, connectionToSink);
}