#include <map>
#include "channelID.hpp"
#include "channelInfo.hpp"
#include "block.hpp"

channelID findPindConcurrently(std::map<channelID, unsigned char> &channelToIndex, std::map<unsigned char, std::set<channelID>> &indexToChannels, unsigned char arraySize, unsigned char &indexOfChosenChannel,
                               unsigned char channelWidth, std::map<channelID, channelInfo> const &channelInformation, std::map<channelID, std::set<std::string>> &relevantChannels, std::set<channelID> &doublyRelevantChannels,
                               std::set<std::string> &reachedBlocks, std::map<std::string, std::shared_ptr<block>> const &blocks);