#ifndef ROUTING_HPP
#define ROUTING_HPP

#include <map>
#include <set>
#include <string>
#include <memory>
#include "net.hpp"
#include "block.hpp"

bool routeNets(unsigned char arraySize, unsigned char channelwidth, std::vector<std::shared_ptr<net>> const &sortedNets, std::map<std::string, std::shared_ptr<block>> const &blocks);

#endif