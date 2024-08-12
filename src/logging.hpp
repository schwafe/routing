#ifndef LOGGING_H
#define LOGGING_H

#include <string>
#include "channel.hpp"
#include "net.hpp"

std::string channelIDToString(channelID channelID);
void printConnections(net net);
void printIndices(std::map<channelID, unsigned char> indices);

#endif