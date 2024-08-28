#include <map>
#include <set>
#include "channel/channel.hpp"
#include "block.hpp"
#include "findResult.hpp"

findResult findPinWithThreads(std::set<unsigned char> tracksToCheck, std::map<unsigned char, std::map<channelID, unsigned char>> &channelToIndexMaps, std::map<unsigned char, std::map<unsigned char, std::set<channelID>>> &indexToChannelsMaps, unsigned char arraySize,
                             unsigned char channelWidth, std::map<channelID, channelInfo> const &channelInformation, std::map<channelID, std::set<std::string>> const &relevantChannels,
                             std::set<channelID> const &doublyRelevantChannels, std::map<std::string, std::shared_ptr<block>> const &blocks);