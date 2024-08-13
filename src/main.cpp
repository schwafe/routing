// #define NDEBUG TODO enable for last version

#include <iostream>
#include <fstream>
#include <regex>
#include <cassert>
#include "constants.hpp"
#include "channel.hpp"
#include "routing.hpp"
#include "block.hpp"

void failIfFalse(bool condition, const unsigned char errorCode, std::string failMessage)
{
    if (!condition)
    {
        std::cerr << failMessage << std::endl;
        exit(errorCode);
    }
}

void failIfInvalidRead(bool valid, std::string failMessage)
{
    failIfFalse(valid, constants::readFailure, failMessage);
}

void readNet(std::string &fileName, std::map<std::string, std::shared_ptr<net>> &netsByNameOfTheSourceBlock, std::map<std::string, std::shared_ptr<net>> &netsByNameOfTheNet,
             std::map<std::string, std::shared_ptr<block>> &blocks)
{
    std::ifstream netFile;
    netFile.open(constants::netPrefix + fileName + constants::netSuffix);

    std::map<std::string, std::string> inputBlockNameByNameOfTheNet;
    std::map<std::string, std::string> outputBlockNameByNameOfTheNet;
    // std::set<std::string> netsConnectedToClock;

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
                // TODO make sure this matches correctly
                if (matches[index] != "open")
                {
                    if (netsByNameOfTheNet.contains(matches[index]))
                    {
                        netsByNameOfTheNet.find(matches[index])->second->setConnectedPin(blockName);
                    }
                    else
                    {
                        std::shared_ptr<net> p_net = std::make_shared<net>();
                        p_net->setConnectedPin(blockName);
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
                p_net = std::make_shared<net>();
                netsByNameOfTheNet.insert(std::make_pair(matches[5], p_net));
            }
            p_net->setSourceBlockName(blockName);
            netsByNameOfTheSourceBlock.insert(std::make_pair(blockName, p_net));

            /*             if (matches[6] != "open")
                        {
                            blocksConnectedToClock.insert(std::make_pair(blockName, std::make_pair(0, p_block)));
                            netsConnectedToClock.insert(matches[5]);
                        } */

            // for a simple architecture (at most one 4-LUT + one FF per clb) subblock line does not provide additional information
            std::getline(netFile, line);
            failIfInvalidRead(std::regex_match(line, matches, constants::clbSubPattern), "No subblock line after the pinlist of clb " + blockName);

            std::getline(netFile, line);
            failIfInvalidRead(line.empty(), "No empty line after the subblock line of clb " + blockName);
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
                netsByNameOfTheNet.find(matches[1])->second->setConnectedPin(blockName);
            }
            else
            {
                std::shared_ptr<net> p_net = std::make_shared<net>();
                p_net->setConnectedPin(blockName);
                netsByNameOfTheNet.insert(std::make_pair(matches[1], p_net));
            }

            std::getline(netFile, line);
            failIfInvalidRead(line.empty(), "No empty line after the pinlist line of output " + blockName);
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
                p_net = std::make_shared<net>();
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
            failIfInvalidRead(line.empty(), "No empty line after the pinlist line of input " + blockName);
        }
        else if (std::regex_match(line, matches, constants::globalPattern))
        {
            /*             clockName = matches[1]; */

            std::getline(netFile, line);
            failIfInvalidRead(line.empty(), "No empty line after global declaration of " + blockName);
        }
        else
        {
            // expecting file to end
            failIfInvalidRead(line.empty(), "Invalid line: '" + line + "'.");

            std::string secLine;
            failIfInvalidRead(!std::getline(netFile, secLine) && netFile.eofbit, "Expected file to end after two empty lines, but instead got this line: '" + secLine + "'.");
            std::cout << "Netfile was successfully read!" << std::endl;
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
}

void readPlace(std::string fileName, unsigned char &arraySize, std::map<std::string, std::shared_ptr<block>> &blocks, auto &blocksConnectedToClock, auto &netsByNameOfTheSourceBlock)
{
    std::string line;
    std::ifstream placeFile;
    placeFile.open(constants::placePrefix + fileName + constants::placementSuffix);

    // netlist and architecture file line
    std::getline(placeFile, line);

    // array size line
    std::smatch matches;
    std::getline(placeFile, line);
    failIfInvalidRead(std::regex_search(line, matches, constants::arraySizePattern), "No array size given or format unknown!");
    arraySize = std::stoi(matches[1]);

    // empty line
    std::getline(placeFile, line);

    bool matched;
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
}

void writeRouting(const std::string &fileName, unsigned char &arraySize, auto &netsByNameOfTheNet, auto &blocks)
{
    std::ofstream routingFile;
    routingFile.open(constants::routingPrefix + fileName + constants::routingSuffix);

    routingFile << "Array size: " << +arraySize << " x " << +arraySize << " logic blocks.\n\nRouting:\n";

    for (auto &netEntry : netsByNameOfTheNet)
    {
        std::shared_ptr<net> p_net = netEntry.second;

        routingFile << "\nNet " << p_net->getIndex() << " (" << netEntry.first << ")\n\n";

        assert(blocks.contains(p_net->getSourceBlockName()));
        std::shared_ptr<block> p_sourceBlock = blocks.find(p_net->getSourceBlockName())->second;
        unsigned char x = p_sourceBlock->getX();
        unsigned char y = p_sourceBlock->getY();
        unsigned char subblockNumber = p_sourceBlock->getSubblockNumber();

        routingFile << "SOURCE (" << +x << "," << +y << ")  ";
        if (p_sourceBlock->getType() == constants::blockTypeCLB)
            routingFile << "Class: " << +constants::outputPinClass << "  \n";
        else
            routingFile << "Pad: " << +subblockNumber << "  \n";

        routingFile << "  OPIN (" << +x << "," << +y << ")  ";
        if (p_sourceBlock->getType() == constants::blockTypeCLB)
            routingFile << "Pin: " << +constants::outputPinNumber << "  \n";
        else
            routingFile << "Pad: " << +subblockNumber << "  \n";

        for (auto &connectionEntry : p_net->getConnectedPinBlockNamesAndTheirRouting())
        {
            assert(blocks.contains(connectionEntry.first));
            std::shared_ptr<block> p_block = blocks.find(connectionEntry.first)->second;
            std::stack<std::pair<channelID, unsigned char>> connection = connectionEntry.second;
            channelID channel;

            assert(!connection.empty());
            do
            {
                channel = connection.top().first;

                routingFile << " CHAN" << channel.getType() << " (" << +channel.getXCoordinate() << ',' << +channel.getYCoordinate() << ")  Track: " << +connection.top().second << "  \n";
                connection.pop();
            } while (!connection.empty());

            if (p_block->getType() == constants::blockTypeCLB)
            {
                routingFile << "  IPIN (" << +p_block->getX() << ',' << +p_block->getY() << ")  Pin: " << +p_block->determinePinNumber(channel) << "  \n";
                routingFile << "  SINK (" << +p_block->getX() << ',' << +p_block->getY() << ")  Class: 0  \n";
            }
            else
            {
                routingFile << "  IPIN (" << +p_block->getX() << ',' << +p_block->getY() << ")  Pad: 0  \n  SINK (" << +p_block->getX() << ',' << +p_block->getY() << ")  Pad: " << p_block->getSubblockNumber() << "  \n\n";
            }
        }
    }
    routingFile.close();
}

void deepCopy(const std::map<std::string, std::shared_ptr<net>> &netsByNameOfTheSourceBlock, auto &copyOfNets, const std::map<std::string, std::shared_ptr<block>> &blocks, auto &copyOfBlocks)
{
    for (auto &entry : netsByNameOfTheSourceBlock)
    {
        net copy = *entry.second;
        copyOfNets.emplace(entry.first, std::make_shared<net>(copy));
    }

    for (auto &entry : blocks)
    {
        block copy = *entry.second;
        copyOfBlocks.emplace(entry.first, std::make_shared<block>(copy));
    }
}

int main(int argc, char *argv[])
{
    failIfFalse(argc == 2, constants::wrongArguments, "Argument count unexpected! Please enter just one argument: the filename for the .net and .place files to be used.");
    std::string fileName = argv[1];

    std::cout << "Program start!" << std::endl;

    unsigned char arraySize;
    std::string clockName;
    std::map<std::string, std::shared_ptr<block>> blocks;
    std::map<std::string, std::shared_ptr<net>> netsByNameOfTheSourceBlock;
    std::map<std::string, std::shared_ptr<net>> netsByNameOfTheNet;
    std::map<std::string, std::pair<unsigned short, std::shared_ptr<block>>> blocksConnectedToClock;

    std::cout << "Reading net file!" << std::endl;
    readNet(fileName, netsByNameOfTheSourceBlock, netsByNameOfTheNet, blocks);

    std::cout << "Reading place file!" << std::endl;
    readPlace(fileName, arraySize, blocks, blocksConnectedToClock, netsByNameOfTheSourceBlock);

    unsigned char channelwidth = constants::startingValuechannelwidth;

    assert(netsByNameOfTheNet.size() == netsByNameOfTheSourceBlock.size());

    std::cout << "arraySize: " << +arraySize << std::endl
              << "channelwidth: " << +channelwidth << std::endl
              << "netCount: " << netsByNameOfTheSourceBlock.size() << std::endl
              << "block count: " << blocks.size() << std::endl;
    /*
    << "blocksConnectedToClock: " << blocksConnectedToClock.size() << std::endl
    << "name of the clock: " << clockName << std::endl; */

    bool success{};
    unsigned char successfulWidth = std::numeric_limits<unsigned char>::max();
    std::map<std::string, std::shared_ptr<net>> tempNetsByNameOfTheSourceBlock{}, finalNetsByNameOfTheSourceBlock{};
    std::map<std::string, std::shared_ptr<block>> tempBlocks{}, finalBlocks{};

    // TODO trying again with a broader channelwidth fails early - 25k lines for 12 tracks, 250 for 13, 230 for 14, 27 for 15 und 2 for 16
    do
    {
        std::cout << "\n----------------\nRouting nets with a channelwidth of " << +channelwidth << " tracks!" << std::endl;
        deepCopy(netsByNameOfTheSourceBlock, tempNetsByNameOfTheSourceBlock, blocks, tempBlocks);
        success = routeNets(arraySize, channelwidth, tempNetsByNameOfTheSourceBlock, tempBlocks);

        if (success)
        {
            successfulWidth = channelwidth;
            finalNetsByNameOfTheSourceBlock = tempNetsByNameOfTheSourceBlock;
            finalBlocks = tempBlocks;
            channelwidth = channelwidth / 2;
            assert(channelwidth > 0);
            std::cout << "Success! Trying again with a channelwidth of " << +channelwidth << " tracks!" << std::endl;
        }
        else
        {
            channelwidth++;
            std::cout << "Failure! Trying again with a channelwidth of " << +channelwidth << " tracks!" << std::endl;
        }

        assert(channelwidth <= 16);
    } while (success || blocks.empty() || channelwidth < successfulWidth);

    std::cout << "Writing routing file!" << std::endl;
    writeRouting(fileName, arraySize, finalNetsByNameOfTheSourceBlock, finalBlocks);

    return constants::success;
}
