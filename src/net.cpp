#include "net.hpp"

#include <cassert>
#include <iostream>

net::net()
{
    net::connectedPinsAndTheirRouting = std::map<std::string, std::stack<std::pair<channelID, unsigned char>>>();
}

channelID net::getSourceChannel()
{
    return sourceChannel;
}

std::string net::getSourceBlockName()
{
    return sourceBlockName;
}

unsigned short net::getIndex()
{
    return index;
}

unsigned short net::getPinCount()
{
    return connectedPinsAndTheirRouting.size();
}

std::map<std::string, std::stack<std::pair<channelID, unsigned char>>> net::getConnectedPinBlockNamesAndTheirRouting()
{
    return connectedPinsAndTheirRouting;
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
    std::stack<std::pair<channelID, unsigned char>> emptyConnection = std::stack<std::pair<channelID, unsigned char>>();
    net::connectedPinsAndTheirRouting.emplace(name, emptyConnection);
}

void net::setConnection(std::string reachedBlock, std::stack<std::pair<channelID, unsigned char>> connectionToSink)
{
    connectedPinsAndTheirRouting.find(reachedBlock)->second = connectionToSink;
}

bool net::allPinsConnected()
{
    for (auto &entry : connectedPinsAndTheirRouting)
    {
        if (entry.second.empty())
        {
            return false;
        }
    }
    return true;
}

std::string net::listConnectedBlocks()
{
    std::string list;
    for (auto &entry : connectedPinsAndTheirRouting)
        list.append(entry.first).append(", ");
    return list.substr(0, list.size() - 2);
}