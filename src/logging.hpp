#ifndef LOGGING_H
#define LOGGING_H

#include <string>
#include <memory>
#include "channel/channel.hpp"
#include "net.hpp"

std::string channelIDToString(channelID const &channelID);
std::string argsToString(int argc, char *argv[]);
std::string listConnectedBlocks(std::shared_ptr<net> const &p_net);
void printConnections(std::shared_ptr<net> const &net);
void printChannelToIndex(std::map<channelID, unsigned char> const &channelToIndex);
void printLogMessage(std::string const &loggingMessage);

#endif