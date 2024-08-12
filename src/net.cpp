#include "net.hpp"
#include <iostream>
#include <cassert>
#include "constants.hpp"

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
    assert(!connectionToSink.empty());
    auto pair = std::make_pair(reachedBlock, std::stack(connectionToSink));
    connectedPinsAndTheirRouting.insert(pair);
        
    assert(!connectionToSink.empty());
    assert(!pair.second.empty());
    assert(!connectedPinsAndTheirRouting.find(reachedBlock)->second.empty()); // <--fails
    
    connectedPinsAndTheirRouting.find(reachedBlock)->second.push(std::make_pair(channelID(1,2,constants::channelTypeX) , 0));
    assert(!connectedPinsAndTheirRouting.find(reachedBlock)->second.empty()); // <--does not fail
}

bool net::allPinsConnected()
{
    std::cout << "netsize: " << connectedPinsAndTheirRouting.size() << "o0 stacksize "<< (int) connectedPinsAndTheirRouting.find("o0")->second.size() <<std::endl;
    for (auto &entry : connectedPinsAndTheirRouting)
    {
        if (entry.second.empty())
        {
            std::cout << "name: " << entry.first << std::endl;
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