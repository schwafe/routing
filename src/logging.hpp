#ifndef LOGGING_H
#define LOGGING_H

#include <string>
#include <memory>
#include "channel.hpp"
#include "net.hpp"

std::string channelIDToString(channelID channelID);
void printConnections(std::shared_ptr<net> net);
void printIndices(std::map<channelID, unsigned char> indices);

#endif