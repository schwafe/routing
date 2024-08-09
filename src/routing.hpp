#ifndef ROUTING_HPP
#define ROUTING_HPP

#include <map>
#include <set>
#include <string>
#include <memory>
#include "net.hpp"
#include "block.hpp"

bool routeNets(unsigned char const &arraySize, unsigned char const &maxTracks, std::map<std::string, std::shared_ptr<net>> &netsByNameOfTheSourceBlock,
               std::map<std::string, std::shared_ptr<block>> &blocks, std::map<std::string, std::pair<unsigned short, std::shared_ptr<block>>> &blocksConnectedToClock, std::string &clockName);

#endif