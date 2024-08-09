#include "net.hpp"

net::net() {}

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
    net::connectedPinsAndTheirRouting.emplace(name, std::stack<std::pair<channelID, unsigned char>>());
}

void net::setConnection(std::string reachedBlock, std::stack<std::pair<channelID, unsigned char>> connectionToSink)
{
    connectedPinsAndTheirRouting.emplace(reachedBlock, connectionToSink);
}