#ifndef LOGGING_H
#define LOGGING_H

#include <string>
#include <memory>
#include "channel.hpp"
#include "net.hpp"

std::string channelIDToString(channelID channelID);
std::string argsToString(int argc, char *argv[]);
std::string listConnectedBlocks(std::shared_ptr<net> p_net);
void printConnections(std::shared_ptr<net> net);
void printIndices(std::map<channelID, unsigned char> indices);
void printLogMessage(std::string loggingMessage);

#endif