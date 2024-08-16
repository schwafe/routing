#include <string>
#include <map>
#include <memory>
#include "net.hpp"
#include "block.hpp"

bool readNet(std::string fileName, std::map<std::string, std::shared_ptr<net>> &netsByNameOfTheSourceBlock, std::map<std::string, std::shared_ptr<net>> &netsByNameOfTheNet,
             std::set<std::string> &netsConnectedToClock, std::set<std::shared_ptr<net>> &globalNets, std::map<std::string, std::shared_ptr<block>> &blocks, std::string &errorMessage);

bool readPlace(std::string fileName, unsigned char &arraySize, std::map<std::string, std::shared_ptr<net>> &netsByNameOfTheSourceBlock, std::map<std::string, std::shared_ptr<block>> &blocks,
               std::string &errorMessage);

void writeRouting(std::string fileName, unsigned char arraySize, std::vector<std::shared_ptr<net>> const &sortedNets, std::set<std::shared_ptr<net>> const &globalNets,
                  std::map<std::string, std::shared_ptr<block>> const &blocks);