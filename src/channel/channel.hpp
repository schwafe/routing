#ifndef CHANNEL_H
#define CHANNEL_H

#include <map>
#include <set>
#include "channelID.hpp"
#include "channelInfo.hpp"

std::map<channelID, channelInfo> generateChannelInformation(unsigned char arraySize);
bool isChannelFull(channelID const &channel, std::map<channelID, channelInfo> const &channelInformation, unsigned char channelWidth);
bool isChannelTrackFree(channelID const &channel, unsigned char track, std::map<channelID, channelInfo> const &channelInformation);
std::set<unsigned char> getFreeTracks(channelID const &channel, std::map<channelID, channelInfo> const &channelInformation, unsigned char channelWidth);
unsigned char findOptimalTrack(channelID const &channel, std::map<channelID, channelInfo> const &channelInformation, unsigned char channelWidth);
void useChannel(channelID const &channel, std::map<channelID, channelInfo> &channelInformation, unsigned char track);
channelID chooseNeighbouringChannel(channelID channel, unsigned char arraySize, std::set<channelID> const &validChannels,
                                                 std::map<channelID, channelInfo> const &channelInformation);

#endif