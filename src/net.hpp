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
    std::multimap<channelID, unsigned char> usedTracks{};
    std::map<std::string, std::stack<std::pair<channelID, unsigned char>>> connectedPinsAndTheirRouting{};

public:
    net();
    net(const net &other);

    unsigned short getPinCount() const;
    channelID getSourceChannel() const;
    std::string getSourceBlockName() const;
    unsigned short getIndex() const;
    std::map<std::string, std::stack<std::pair<channelID, unsigned char>>> getConnectedPinBlockNamesAndTheirRouting() const;
    bool usedChannel(const channelID &channel) const;

    unsigned char chooseUsedTrack(const channelID &channel, const unsigned char &optimalTrack) const;
    bool allPinsConnected() const;
    std::string listConnectedBlocks() const;

    void setSourceChannel(channelID sourceChannel);
    void setSourceBlockName(std::string sourceBlockName);
    void setIndex(unsigned short index);
    void setConnectedPin(std::string blockName);
    void setUsedTrack(const channelID &channel, const unsigned char &track);
    void setConnection(std::string reachedBlock, std::stack<std::pair<channelID, unsigned char>> connectionToSink);
};

#endif