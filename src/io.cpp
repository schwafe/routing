#include <fstream>
#include <cassert>
#include "io.hpp"
#include "logging.hpp"

bool readNet(std::string const &fileName, std::map<std::string, std::shared_ptr<net>> &netsByNameOfTheSourceBlock, std::map<std::string, std::shared_ptr<net>> &netsByNameOfTheNet,
             std::set<std::string> &netsConnectedToClock, std::set<std::shared_ptr<net>> &globalNets, std::map<std::string, std::shared_ptr<block>> &blocks, std::string &errorMessage)
{
    std::ifstream netFile;
    netFile.open(fileName);

    std::string line;

    while (std::getline(netFile, line))
    {
        std::smatch matches;
        std::string blockName;
        if (std::regex_match(line, matches, constants::clbPattern))
        {
            blockName = matches[1];

            std::shared_ptr<block> p_block = std::make_shared<block>(constants::blockTypeCLB);
            blocks.insert(std::make_pair(blockName, p_block));

            std::getline(netFile, line);
            std::regex_match(line, matches, constants::clbPinPattern);
            for (unsigned char index = 1; index <= 4; index++)
            {
                if (matches[index] != "open")
                {
                    if (auto it = netsByNameOfTheNet.find(matches[index]); it != netsByNameOfTheNet.end())
                        it->second->addConnectedBlock(blockName);
                    else
                    {
                        std::shared_ptr<net> p_net = std::make_shared<net>(matches[index]);
                        p_net->addConnectedBlock(blockName);
                        netsByNameOfTheNet.insert(std::make_pair(matches[index], p_net));
                    }
                }
            }

            std::shared_ptr<net> p_net;
            if (auto it = netsByNameOfTheNet.find(matches[5]); it != netsByNameOfTheNet.end())
                p_net = it->second;
            else
            {
                p_net = std::make_shared<net>(matches[5]);
                netsByNameOfTheNet.insert(std::make_pair(matches[5], p_net));
            }
            p_net->setSourceBlockName(blockName);
            netsByNameOfTheSourceBlock.insert(std::make_pair(blockName, p_net));

            if (matches[6] != "open")
            {
                netsConnectedToClock.insert(blockName);

                if (auto it = netsByNameOfTheNet.find(matches[6]); it != netsByNameOfTheNet.end())
                    it->second->addConnectedBlock(blockName);
                else
                {
                    std::shared_ptr<net> p_net = std::make_shared<net>(matches[6]);
                    p_net->addConnectedBlock(blockName);
                    netsByNameOfTheNet.insert(std::make_pair(matches[6], p_net));
                }
            }

            // for a simple architecture (at most one 4-LUT + one FF per clb) subblock line does not provide additional information
            std::getline(netFile, line);
            if (!std::regex_match(line, matches, constants::clbSubPattern))
            {
                errorMessage = "No subblock line after the pinlist of clb " + blockName;
                return false;
            }

            std::getline(netFile, line);
            if (!line.empty())
            {
                errorMessage = "No empty line after the subblock line of clb " + blockName;
                return false;
            }
        }
        else if (std::regex_match(line, matches, constants::outputPattern))
        {
            blockName = matches[1];
            blocks.insert(std::make_pair(blockName, std::make_shared<block>(constants::blockTypeOutput)));

            std::getline(netFile, line);
            std::regex_match(line, matches, constants::padPinPattern);

            if (auto it = netsByNameOfTheNet.find(matches[1]); it != netsByNameOfTheNet.end())
                it->second->addConnectedBlock(blockName);
            else
            {
                std::shared_ptr<net> p_net = std::make_shared<net>(matches[1]);
                p_net->addConnectedBlock(blockName);
                netsByNameOfTheNet.insert(std::make_pair(matches[1], p_net));
            }

            std::getline(netFile, line);
            if (!line.empty())
            {
                errorMessage = "No empty line after the pinlist line of output " + blockName;
                return false;
            }
        }
        else if (std::regex_match(line, matches, constants::inputPattern))
        {
            blockName = matches[1];
            std::shared_ptr<block> p_block = std::make_shared<block>(constants::blockTypeInput);

            blocks.insert(std::make_pair(blockName, p_block));

            std::getline(netFile, line);
            std::regex_match(line, matches, constants::padPinPattern);

            std::shared_ptr<net> p_net;
            if (auto it = netsByNameOfTheNet.find(matches[1]); it != netsByNameOfTheNet.end())
                p_net = it->second;
            else
            {
                p_net = std::make_shared<net>(matches[1]);
                netsByNameOfTheNet.insert(std::make_pair(matches[1], p_net));
            }
            p_net->setSourceBlockName(blockName);
            netsByNameOfTheSourceBlock.insert(std::make_pair(blockName, p_net));

            std::getline(netFile, line);
            if (!line.empty())
            {
                errorMessage = "No empty line after the pinlist line of input " + blockName;
                return false;
            }
        }
        else if (std::regex_match(line, matches, constants::globalPattern))
        {
            std::shared_ptr<net> p_net;
            if (auto it = netsByNameOfTheNet.find(matches[1]); it != netsByNameOfTheNet.end())
                p_net = it->second;
            else
            {
                p_net = std::make_shared<net>(matches[1]);
                netsByNameOfTheNet.insert(std::make_pair(matches[1], p_net));
            }
            assert(!globalNets.contains(p_net));
            globalNets.emplace(p_net);

            std::getline(netFile, line);
            if (!line.empty())
            {
                errorMessage = "No empty line after global declaration of " + blockName;
                return false;
            }
        }
        else
        {
            // expecting file to end
            if (!line.empty())
            {
                errorMessage = "Invalid line: '" + line + "'.";
                return false;
            }

            std::string secLine;

            if (std::getline(netFile, secLine))
            {
                errorMessage = "Expected file to end after two empty lines, but instead got this line: '" + secLine + "'.";
                return false;
            }
            assert(netFile.eofbit);
        }
    }

    netFile.close();
    return true;
}

bool readPlace(std::string const &fileName, unsigned char &arraySize, std::map<std::string, std::shared_ptr<net>> &netsByNameOfTheSourceBlock,
               std::map<std::string, std::shared_ptr<block>> &blocks, std::string &errorMessage)
{
    std::ifstream placeFile;
    placeFile.open(fileName);

    std::string line;

    // netlist and architecture file line
    std::getline(placeFile, line);

    // array size line
    std::smatch matches;
    std::getline(placeFile, line);

    if (!std::regex_search(line, matches, constants::arraySizePattern))
    {
        errorMessage = "No array size was given or the format is unknown!";
        return false;
    }

    arraySize = std::stoi(matches[1]);

    // empty line
    std::getline(placeFile, line);

    int i = 0;
    do
    {
        std::getline(placeFile, line);

        while (std::regex_match(line, constants::commentPattern))
        {
            // skip comment lines
            std::getline(placeFile, line);
        }

        std::smatch matches;
        if (std::regex_search(line, matches, constants::placePattern))
        {
            unsigned char x = std::stoi(matches[2]);
            unsigned char y = std::stoi(matches[3]);
            std::string blockName = matches[1];

            assert(blocks.contains(blockName));
            std::shared_ptr<block> p_block = blocks.find(blockName)->second;
            p_block->initialise(x, y, std::stoi(matches[4]));

            if (p_block->getType() != constants::blockTypeOutput)
            {
                channelID sourceChannel;
                if (p_block->getType() == constants::blockTypeCLB || y == arraySize + 1)
                {
                    sourceChannel = channelID(x, y - 1, constants::channelTypeX);
                }
                else if (x == 0)
                {
                    sourceChannel = channelID(x, y, constants::channelTypeY);
                }
                else if (x == arraySize + 1)
                {
                    sourceChannel = channelID(x - 1, y, constants::channelTypeY);
                }
                else if (y == 0)
                {
                    sourceChannel = channelID(x, y, constants::channelTypeX);
                }

                assert(netsByNameOfTheSourceBlock.contains(blockName));
                netsByNameOfTheSourceBlock.find(blockName)->second->setSourceChannel(sourceChannel);
            }
        }

    } while (!(line.empty() && placeFile.eofbit));

    placeFile.close();
    return true;
}

void writeRouting(std::string const &fileName, unsigned char arraySize, std::vector<std::shared_ptr<net>> const &sortedNets, std::set<std::shared_ptr<net>> const &globalNets,
                  std::map<std::string, std::shared_ptr<block>> const &blocks)
{
    std::ofstream routingFile;
    routingFile.open(fileName);

    routingFile << "Array size: " << +arraySize << " x " << +arraySize << " logic blocks.\n\nRouting:";

    unsigned char globalIndex = 0;

    for (std::shared_ptr<net> p_net : globalNets)
    {
        routingFile << "\n\nNet " << +(globalIndex++) << " (" << p_net->getName() << "): global net connecting:\n\n";

        assert(blocks.contains(p_net->getSourceBlockName()));
        std::shared_ptr<block> p_block = blocks.find(p_net->getSourceBlockName())->second;
        routingFile << "Block " << p_net->getSourceBlockName() << " at (" << +p_block->getX() << ", " << +p_block->getY() << "), Pin class ";
        if (p_block->getType() == constants::blockTypeCLB)
            routingFile << +constants::clockPinClass;
        else
            routingFile << +constants::irrelevantPinClass;
        routingFile << ".\n";

        for (std::string connectedBlockName : p_net->getNamesOfConnectedBlocks())
        {
            assert(blocks.contains(connectedBlockName));
            std::shared_ptr<block> p_block = blocks.find(connectedBlockName)->second;
            routingFile << "Block " << connectedBlockName << " at (" << +p_block->getX() << ", " << +p_block->getY() << "), Pin class ";
            if (p_block->getType() == constants::blockTypeCLB)
                routingFile << +constants::clockPinClass;
            else
                routingFile << +constants::irrelevantPinClass;
            routingFile << ".\n";
        }
    }

    for (unsigned short index = 0; index < sortedNets.size(); index++)
    {
        std::shared_ptr<net> p_net = sortedNets[index];
        std::set<unsigned char> usedTracksAtSourceChannel{};

        routingFile << "\n\nNet " << (index + globalIndex) << " (" << p_net->getName() << ")\n\n";

        assert(blocks.contains(p_net->getSourceBlockName()));
        std::shared_ptr<block> p_sourceBlock = blocks.find(p_net->getSourceBlockName())->second;
        unsigned char subblockNumber = p_sourceBlock->getSubblockNumber();

        routingFile << "SOURCE (" << +p_sourceBlock->getX() << "," << +p_sourceBlock->getY() << ")  ";
        if (p_sourceBlock->getType() == constants::blockTypeCLB)
            routingFile << "Class: " << +constants::outputPinClass << "  \n";
        else
            routingFile << "Pad: " << +subblockNumber << "  \n";

        for (auto &connectionEntry : p_net->getConnectionsByRoutingOrder())
        {
            assert(blocks.contains(connectionEntry.first));
            std::shared_ptr<block> p_block = blocks.find(connectionEntry.first)->second;
            std::vector<std::pair<channelID, unsigned char>> connection = connectionEntry.second;
            channelID channel;

            assert(!connection.empty());

            for (auto revIt = connection.rbegin(); revIt != connection.rend(); revIt++)
            {
                channel = revIt->first;
                unsigned char track = revIt->second;

                if (channel == p_net->getSourceChannel())
                {
                    assert(revIt == connection.rbegin());

                    if (!usedTracksAtSourceChannel.contains(track))
                    {
                        routingFile << "  OPIN (" << +p_sourceBlock->getX() << "," << +p_sourceBlock->getY() << ")  ";
                        if (p_sourceBlock->getType() == constants::blockTypeCLB)
                            routingFile << "Pin: " << +constants::outputPinNumber << "  \n";
                        else
                            routingFile << "Pad: " << +subblockNumber << "  \n";

                        usedTracksAtSourceChannel.emplace(track);
                    }
                }

                routingFile << " CHAN" << channel.getType() << " (" << +channel.getXCoordinate() << ',' << +channel.getYCoordinate() << ")  Track: " << +track << "  \n";
            }

            if (p_block->getType() == constants::blockTypeCLB)
            {
                routingFile << "  IPIN (" << +p_block->getX() << ',' << +p_block->getY() << ")  Pin: " << +p_block->determinePinNumber(channel) << "  \n";
                routingFile << "  SINK (" << +p_block->getX() << ',' << +p_block->getY() << ")  Class: 0  \n";
            }
            else
            {
                routingFile << "  IPIN (" << +p_block->getX() << ',' << +p_block->getY() << ")  Pad: " << +p_block->getSubblockNumber() << "  \n  SINK (" << +p_block->getX() << ',' << +p_block->getY() << ")  Pad: "
                            << +p_block->getSubblockNumber() << "  \n";
            }
        }
    }

    routingFile.close();
}