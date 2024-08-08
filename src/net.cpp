#include "net.hpp"

channelID net::getStartingChannel()
{
    return net::startingChannel;
}

unsigned short net::getPinCount()
{
    return net::connectedPinsAndTheirRouting.size();
}