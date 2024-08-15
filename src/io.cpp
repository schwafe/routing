#include <iostream>
#include <fstream>
#include <cassert>
#include "io.hpp"
#include "logging.hpp"

bool readNet(std::string fileName, std::map<std::string, std::shared_ptr<net>> &netsByNameOfTheSourceBlock, std::map<std::string, std::shared_ptr<net>> &netsByNameOfTheNet,
             std::map<std::string, std::shared_ptr<block>> &blocks, std::string &errorMessage)
{
    std::ifstream netFile;
    netFile.open(constants::netPrefix + fileName + constants::netSuffix);

    std::map<std::string, std::string> inputBlockNameByNameOfTheNet{}, outputBlockNameByNameOfTheNet{};
    std::set<std::string> netsConnectedToClock{}, blocksConnectedToClock{};

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
                    if (netsByNameOfTheNet.contains(matches[index]))
                    {
                        netsByNameOfTheNet.find(matches[index])->second->addSinkBlock(blockName);
                    }
                    else
                    {
                        std::shared_ptr<net> p_net = std::make_shared<net>(matches[index]);
                        p_net->addSinkBlock(blockName);
                        netsByNameOfTheNet.insert(std::make_pair(matches[index], p_net));
                    }
                }
            }

            std::shared_ptr<net> p_net;
            if (netsByNameOfTheNet.contains(matches[5]))
            {
                p_net = netsByNameOfTheNet.find(matches[5])->second;
            }
            else
            {
                p_net = std::make_shared<net>(matches[5]);
                netsByNameOfTheNet.insert(std::make_pair(matches[5], p_net));
            }
            p_net->setSourceBlockName(blockName);
            netsByNameOfTheSourceBlock.insert(std::make_pair(blockName, p_net));

            /* if (matches[6] != "open") TODO index of nets connected to clock can be discovered during routingNets()
            {
                int netIndex = -1;
                blocksConnectedToClock.insert(std::make_pair(blockName, std::make_pair(netIndex, p_block)));
                netsConnectedToClock.insert(matches[5]);
            } */

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

            outputBlockNameByNameOfTheNet.emplace(matches[1], blockName);

            if (netsByNameOfTheNet.contains(matches[1]))
            {
                netsByNameOfTheNet.find(matches[1])->second->addSinkBlock(blockName);
            }
            else
            {
                std::shared_ptr<net> p_net = std::make_shared<net>(matches[1]);
                p_net->addSinkBlock(blockName);
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

            inputBlockNameByNameOfTheNet.emplace(matches[1], blockName);

            std::shared_ptr<net> p_net;
            if (netsByNameOfTheNet.contains(matches[1]))
            {
                p_net = netsByNameOfTheNet.find(matches[1])->second;
            }
            else
            {
                p_net = std::make_shared<net>(matches[1]);
                netsByNameOfTheNet.insert(std::make_pair(matches[1], p_net));
            }
            p_net->setSourceBlockName(blockName);
            netsByNameOfTheSourceBlock.insert(std::make_pair(blockName, p_net));

            /*             if (matches[6] != "open")
                        {
                            blocksConnectedToClock.insert(std::make_pair(blockName, std::make_pair(0, p_block)));
                            netsConnectedToClock.insert(matches[5]);
                        } */

            std::getline(netFile, line);
            if (!line.empty())
            {
                errorMessage = "No empty line after the pinlist line of input " + blockName;
                return false;
            }
        }
        else if (std::regex_match(line, matches, constants::globalPattern))
        {
            /*             clockName = matches[1]; */

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

    /*     for (std::string netName : netsConnectedToClock)
        {
            assert(outputBlockNameByNameOfTheNet.contains(netName));
            std::string outputBlockName = outputBlockNameByNameOfTheNet.find(netName)->second;
            assert(blocks.contains(outputBlockName));
            std::shared_ptr<block> p_block = blocks.find(outputBlockName)->second;
            blocksConnectedToClock.insert(std::make_pair(outputBlockName, std::make_pair(0, p_block)));
        }

        if (clockName != "")
        {
            assert(netsByNameOfTheNet.contains(clockName) && netsByNameOfTheSourceBlock.contains(clockName))
            netsByNameOfTheNet.erase(clockName);
            netsByNameOfTheSourceBlock.erase(inputBlockNameByNameOfTheNet.find(clockName)->second);
        } */

    netFile.close();
    return true;
}

bool readPlace(std::string fileName, unsigned char &arraySize, std::map<std::string, std::shared_ptr<net>> &netsByNameOfTheSourceBlock, std::map<std::string, std::shared_ptr<block>> &blocks,
               std::map<std::string, std::pair<unsigned short, std::shared_ptr<block>>> &blocksConnectedToClock, std::string &errorMessage)
{
    std::ifstream placeFile;
    placeFile.open(constants::placePrefix + fileName + constants::placementSuffix);

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

void writeRouting(std::string fileName, unsigned char arraySize, std::vector<std::shared_ptr<net>> const &sortedNets, std::map<std::string, std::shared_ptr<block>> const &blocks)
{
    std::ofstream routingFile;
    routingFile.open(constants::routingPrefix + fileName + constants::routingSuffix);

    routingFile << "Array size: " << +arraySize << " x " << +arraySize << " logic blocks.\n\nRouting:\n";

    for (unsigned short index = 0; index < sortedNets.size(); index++)
    {
        std::shared_ptr<net> p_net = sortedNets[index];

        routingFile << "\nNet " << index << " (" << p_net->getName() << ")\n\n";

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

                if (channel == p_net->getSourceChannel())
                {
                    assert(revIt == connection.rbegin());

                    routingFile << "  OPIN (" << +p_sourceBlock->getX() << "," << +p_sourceBlock->getY() << ")  ";
                    if (p_sourceBlock->getType() == constants::blockTypeCLB)
                        routingFile << "Pin: " << +constants::outputPinNumber << "  \n";
                    else
                        routingFile << "Pad: " << +subblockNumber << "  \n";
                }

                routingFile << " CHAN" << channel.getType() << " (" << +channel.getXCoordinate() << ',' << +channel.getYCoordinate() << ")  Track: " << +revIt->second << "  \n";
            }

            if (p_block->getType() == constants::blockTypeCLB)
            {
                routingFile << "  IPIN (" << +p_block->getX() << ',' << +p_block->getY() << ")  Pin: " << +p_block->determinePinNumber(channel) << "  \n";
                routingFile << "  SINK (" << +p_block->getX() << ',' << +p_block->getY() << ")  Class: 0  \n";
            }
            else
            {
                routingFile << "  IPIN (" << +p_block->getX() << ',' << +p_block->getY() << ")  Pad: 0  \n  SINK (" << +p_block->getX() << ',' << +p_block->getY() << ")  Pad: "
                            << +p_block->getSubblockNumber() << "  \n";
            }
        }
        routingFile << "\n";
    }
    routingFile.close();
}