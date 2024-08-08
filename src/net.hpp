#ifndef NET_H
#define NET_H

#include <map>
#include <set>
#include <string>
#include <stack>
#include "channel.hpp"

class net
{
    std::string input;
    channelID startingChannel;
    std::map<std::string, std::stack<channelID>> connectedPinsAndTheirRouting;

    unsigned short getPinCount();

    channelID getStartingChannel();
};

#endif