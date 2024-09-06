#ifndef CHANNEL_H
#define CHANNEL_H

#include <map>
#include <set>
#include "channelID.hpp"
#include "channelInfo.hpp"

bool isChannelFull(channelID const &channel, std::map<channelID, channelInfo> const &channelInformation, unsigned char channelWidth);
bool isChannelTrackFree(channelID const &channel, unsigned char track, std::map<channelID, channelInfo> const &channelInformation);

std::map<channelID, channelInfo> generateChannelInformation(unsigned char arraySize);
std::set<unsigned char> generateFreeTracks(channelID const &channel, std::map<channelID, channelInfo> const &channelInformation, unsigned char channelWidth);
unsigned char findOptimalTrack(channelID const &channel, std::map<channelID, channelInfo> const &channelInformation, unsigned char channelWidth);
channelID chooseNeighbouringChannel(channelID channel, unsigned char arraySize, std::set<channelID> const &validChannels,
                                                 std::map<channelID, channelInfo> const &channelInformation);

void updateChannelInfo(channelID const &channel, std::map<channelID, channelInfo> &channelInformation, unsigned char track);


#endif