#ifndef ROUTING_HPP
#define ROUTING_HPP

#include <map>
#include <set>
#include <string>
#include <memory>
#include "net.hpp"
#include "block.hpp"

unsigned short routeNets(unsigned char arraySize, unsigned char channelwidth, std::vector<std::shared_ptr<net>> const &nets, std::map<std::string, std::shared_ptr<block>> const &blocks);
bool handleChannelWithIndex(channelID channel, unsigned char indexOfChannel, unsigned char expectedIndex, unsigned char track, std::map<channelID, unsigned char> &channelToIndex, std::map<unsigned char, std::set<channelID>> &indexToChannels,
                             unsigned char &indexOfChosenChannel, std::map<channelID, std::set<std::string>> const &relevantChannels, std::set<channelID> const &doublyRelevantChannels,
                             bool &relevantChannelFound, channelID &firstRelChan, unsigned char &indexOfFirstRelChan, std::map<std::string, std::shared_ptr<block>> const &blocks,
                             std::map<channelID, channelInfo> const &channelInformation);
bool handleChannelWithoutIndex(channelID channel, unsigned char indexOfChannel, unsigned char track, std::map<channelID, unsigned char> &channelToIndex, std::map<unsigned char, std::set<channelID>> &indexToChannels,
                                unsigned char &indexOfChosenChannel, std::map<channelID, std::set<std::string>> const &relevantChannels, std::set<channelID> const &doublyRelevantChannels,
                                bool &relevantChannelFound, channelID &firstRelChan, unsigned char &indexOfFirstRelChan, std::map<std::string, std::shared_ptr<block>> const &blocks);

#endif