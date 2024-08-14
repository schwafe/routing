#include "net.hpp"

#include <cassert>
#include <iostream>

net::net() {}

net::net(const net &other)
{
    sourceBlockName = other.sourceBlockName;
    sourceChannel = other.sourceChannel;

    for (auto &entry : other.connectedPinsAndTheirRouting)
        connectedPinsAndTheirRouting.emplace(entry.first, std::stack<std::pair<channelID, unsigned char>>{});
}

channelID net::getSourceChannel() const
{
    return sourceChannel;
}

std::string net::getSourceBlockName() const
{
    return sourceBlockName;
}

unsigned short net::getIndex() const
{
    return index;
}

unsigned short net::getPinCount() const
{
    return connectedPinsAndTheirRouting.size();
}

bool net::usedChannel(const channelID &channel) const
{
    return usedTracks.contains(channel);
}

std::map<std::string, std::stack<std::pair<channelID, unsigned char>>> net::getConnectedPinBlockNamesAndTheirRouting() const
{
    return connectedPinsAndTheirRouting;
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
    for (auto &entry : connectedPinsAndTheirRouting)
        if (entry.second.empty())
            return false;

    return true;
}

std::string net::listConnectedBlocks() const
{
    std::string list;
    for (auto &entry : connectedPinsAndTheirRouting)
        list.append(entry.first).append(", ");
    return list.substr(0, list.size() - 2);
}

void net::setSourceChannel(channelID sourceChannel)
{
    net::sourceChannel = sourceChannel;
}

void net::setSourceBlockName(std::string sourceBlockName)
{
    net::sourceBlockName = sourceBlockName;
}

void net::setIndex(unsigned short index)
{
    net::index = index;
}

void net::setConnectedPin(std::string name)
{
    net::connectedPinsAndTheirRouting.emplace(name, std::stack<std::pair<channelID, unsigned char>>{});
}

void net::setUsedTrack(const channelID &channel, const unsigned char &track)
{
    usedTracks.emplace(channel, track);
}

void net::setConnection(std::string reachedBlock, std::stack<std::pair<channelID, unsigned char>> connectionToSink)
{
    assert(connectedPinsAndTheirRouting.contains(reachedBlock));
    connectedPinsAndTheirRouting.find(reachedBlock)->second = connectionToSink;
}