#ifndef CHANNEL_H
#define CHANNEL_H

#include <map>
#include <set>
#include "channelID.hpp"
#include "channelInfo.hpp"

std::map<channelID, channelInfo> generateChannelInformation(unsigned char arraySize);
bool isChannelFull(channelID const &channel, std::map<channelID, channelInfo> const &channelInformation, unsigned char channelWidth);
unsigned char useChannel(channelID const &channel, std::map<channelID, channelInfo> &channelInformation, unsigned char optimalTrack);
channelID chooseNeighbouringChannel(channelID channel, std::set<channelID> const &validChannels, unsigned char currentTrack, std::map<channelID, channelInfo> const &channelInformation);

#endif