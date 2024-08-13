#ifndef NET_H
#define NET_H

#include <map>
#include <set>
#include <string>
#include <stack>
#include <limits>
#include "channel.hpp"

class net
{
    channelID sourceChannel{};
    std::string sourceBlockName{};
    unsigned short index = std::numeric_limits<unsigned short>::max();
    std::map<std::string, std::stack<std::pair<channelID, unsigned char>>> connectedPinsAndTheirRouting{};

public:
    net();
    net(const net &other);
    unsigned short getPinCount();
    channelID getSourceChannel();
    std::string getSourceBlockName();
    unsigned short getIndex();
    std::map<std::string, std::stack<std::pair<channelID, unsigned char>>> getConnectedPinBlockNamesAndTheirRouting();
    void setSourceChannel(channelID sourceChannel);
    void setSourceBlockName(std::string sourceBlockName);
    void setIndex(unsigned short index);
    void setConnectedPin(std::string blockName);
    void setConnection(std::string reachedBlock, std::stack<std::pair<channelID, unsigned char>> connectionToSink);
    bool allPinsConnected();
    std::string listConnectedBlocks();
};

#endif