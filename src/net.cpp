#include "net.hpp"

net::net() {}

net::net(channelID source)
{
    net::source = source;
}

channelID net::getSource()
{
    return source;
}

unsigned short net::getPinCount()
{
    return connectedPinsAndTheirRouting.size();
}

std::map<std::string, std::stack<channelID>> &net::getConnectedPinsAndTheirRouting()
{
    return connectedPinsAndTheirRouting;
}

void net::setSource(channelID source)
{
    net::source = source;
}

void net::setConnectedPin(std::string name)
{
    net::connectedPinsAndTheirRouting.emplace(name, std::stack<channelID>());
}

void net::setConnection(std::string reachedBlock, std::stack<channelID> connectionToSink)
{
    connectedPinsAndTheirRouting.emplace(reachedBlock, connectionToSink);
}