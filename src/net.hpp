#ifndef NET_H
#define NET_H

#include <map>
#include <set>
#include <string>
#include <stack>
#include "channel.hpp"

class net
{
    channelID source;
    std::map<std::string, std::stack<channelID>> connectedPinsAndTheirRouting;

public:
    net();
    net(channelID source);
    unsigned short getPinCount();
    channelID getSource();
    std::map<std::string, std::stack<channelID>> &getConnectedPinsAndTheirRouting();
    void setSource(channelID source);
    void setConnectedPin(std::string name);
    void setConnection(std::string reachedBlock, std::stack<channelID> connectionToSink);
};

#endif