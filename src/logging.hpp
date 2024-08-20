#ifndef LOGGING_H
#define LOGGING_H

#include <string>
#include <memory>
#include "channel.hpp"
#include "net.hpp"

std::string channelIDToString(channelID const &channelID);
std::string argsToString(int argc, char *argv[]);
std::string listConnectedBlocks(std::shared_ptr<net> const &p_net);
void printConnections(std::shared_ptr<net> const &net);
void printIndices(std::map<channelID, unsigned char> const &indices);
void printLogMessage(std::string const &loggingMessage);

#endif