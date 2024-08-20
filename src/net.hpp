#ifndef NET_H
#define NET_H

#include <map>
#include <set>
#include <string>
#include <stack>
#include <limits>
#include <vector>
#include "channel.hpp"

class net
{
    std::string name{};
    channelID sourceChannel{};
    std::string sourceBlockName{};
    std::set<std::string> sinkBlockNames{};
    std::multimap<channelID, unsigned char> usedTracks{};
    std::vector<std::pair<std::string, std::vector<std::pair<channelID, unsigned char>>>> connectionsByRoutingOrder{};

public:
    net(std::string name);
    net(const net &other);

    std::string getName() const;
    unsigned short getSinkBlockCount() const;
    channelID getSourceChannel() const;
    std::string getSourceBlockName() const;
    std::set<std::string> getSinkBlockNames() const;
    std::vector<std::pair<std::string, std::vector<std::pair<channelID, unsigned char>>>> getConnectionsByRoutingOrder() const;

    bool usedChannel(const channelID &channel) const;
    bool allPinsConnected() const;
    unsigned char chooseUsedTrack(const channelID &channel, unsigned char optimalTrack) const;

    void setSourceChannel(channelID sourceChannel);
    void setSourceBlockName(std::string sourceBlockName);
    void addSinkBlock(std::string sinkBlockName);
    void setUsedTrack(channelID channel, unsigned char track);
    void setConnection(std::string sinkBlockName, std::vector<std::pair<channelID, unsigned char>> connectionToSink);
};

#endif