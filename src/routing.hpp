#ifndef ROUTING_HPP
#define ROUTING_HPP

#include <map>
#include <set>
#include <string>
#include "net.hpp"

void routeNets(std::map<unsigned short, net>& nets, unsigned char& maxTracks);

#endif