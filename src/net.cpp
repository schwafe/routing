#include "net.hpp"

#include <cassert>

net::net(std::string name)
{
    net::name = name;
}

net::net(const net &other)
{
    name = other.name;
    sourceBlockName = other.sourceBlockName;
    sourceChannel = other.sourceChannel;
    namesOfConnectedBlocks = other.namesOfConnectedBlocks;
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

unsigned short net::getConnectedBlockCount() const
{
    return namesOfConnectedBlocks.size();
}

std::set<unsigned char> net::getUsedTracksAtSourceChannel() const
{
    std::set<unsigned char> usedTracksAtSourceChannel{};
    auto range = usedTracks.equal_range(sourceChannel);
    for (auto it = range.first; it != range.second; it++)
    {
        usedTracksAtSourceChannel.insert(it->second);
    }
    return usedTracksAtSourceChannel;
}

bool net::usedChannelTrackCombination(const channelID &channel, unsigned char track) const
{
    auto range = usedTracks.equal_range(channel);
    for (auto it = range.first; it != range.second; it++)
    {
        if (it->second == track)
            return true;
    }
    return false;
}

std::set<std::string> net::getNamesOfConnectedBlocks() const
{
    return namesOfConnectedBlocks;
}

std::vector<std::pair<std::string, std::pair<unsigned char, std::vector<channelID>>>> net::getConnectionsByRoutingOrder() const
{
    return connectionsByRoutingOrder;
}

unsigned char net::chooseUsedTrack(const channelID &channel, unsigned char optimalTrack) const
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
    return namesOfConnectedBlocks.size() == connectionsByRoutingOrder.size();
}

void net::setSourceChannel(channelID sourceChannel)
{
    net::sourceChannel = sourceChannel;
}

void net::setSourceBlockName(std::string sourceBlockName)
{
    net::sourceBlockName = sourceBlockName;
}

void net::addConnectedBlock(std::string connectedBlockName)
{
    assert(!namesOfConnectedBlocks.contains(connectedBlockName));
    namesOfConnectedBlocks.emplace(connectedBlockName);
}

void net::setUsedTrack(channelID channel, unsigned char track)
{
    usedTracks.emplace(channel, track);
}

void net::setConnection(std::string connectedBlockName, unsigned char track, std::vector<channelID> connectionToBlock)
{
    assert(namesOfConnectedBlocks.contains(connectedBlockName));
    connectionsByRoutingOrder.emplace_back(connectedBlockName, std::make_pair(track, connectionToBlock));
}