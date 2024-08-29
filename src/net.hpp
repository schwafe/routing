#ifndef NET_H
#define NET_H

#include <map>
#include <set>
#include <string>
#include <stack>
#include <limits>
#include <vector>
#include "channel/channel.hpp"

class net
{
    std::string name{};
    channelID sourceChannel{};
    std::string sourceBlockName{};
    std::set<std::string> namesOfConnectedBlocks{};
    std::multimap<channelID, unsigned char> usedTracks{};
    std::vector<std::pair<std::string, std::pair<unsigned char, std::vector<channelID>>>> connectionsByRoutingOrder{};

public:
    net(std::string name) : name(std::move(name)) {};

    std::string getName() const;
    unsigned short getConnectedBlockCount() const;
    channelID getSourceChannel() const;
    std::string getSourceBlockName() const;
    std::set<std::string> getNamesOfConnectedBlocks() const;
    std::vector<std::pair<std::string, std::pair<unsigned char, std::vector<channelID>>>> getConnectionsByRoutingOrder() const;

    std::set<unsigned char> findUsedTracksAtSourceChannel() const;
    bool usedChannelTrackCombination(const channelID &channel, unsigned char track) const;
    bool allPinsConnected() const;
    unsigned char chooseUsedTrack(const channelID &channel, unsigned char optimalTrack) const;

    void setSourceChannel(channelID sourceChannel);
    void setSourceBlockName(std::string sourceBlockName);
    void addConnectedBlock(std::string connectedBlockName);
    void setUsedTrack(channelID channel, unsigned char track);
    void setConnection(std::string connectedBlockName, unsigned char track, std::vector<channelID> connectionToBlock);
};

#endif