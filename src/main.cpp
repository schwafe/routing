// #define NDEBUG TODO enable for last version

#include <iostream>
#include <regex>
#include <cassert>
#include "constants.hpp"
#include "io.hpp"
#include "channel.hpp"
#include "routing.hpp"
#include "block.hpp"
#include "logging.hpp"

void abortIfTrue(bool condition, unsigned char errorCode, std::string errorMessage)
{
    if (condition)
    {
        std::cerr << errorMessage << std::endl;
        exit(errorCode);
    }
}

std::vector<std::shared_ptr<net>> sortNets(std::map<std::string, std::shared_ptr<net>> const &netsByNameOfTheSourceBlock)
{
    //TODO nets connected to the clock go first

    std::multimap<unsigned char, std::shared_ptr<net>> netsGroupedBySinkBlockCount{};
    unsigned char highestSinkBlockCount = 0;

    for (auto entry : netsByNameOfTheSourceBlock)
    {
        std::shared_ptr<net> p_net(entry.second);
        netsGroupedBySinkBlockCount.insert(std::make_pair(p_net->getSinkBlockCount(), p_net));

        if (highestSinkBlockCount < p_net->getSinkBlockCount())
            highestSinkBlockCount = p_net->getSinkBlockCount();
    }

    assert(!netsGroupedBySinkBlockCount.contains(0));

    std::vector<std::shared_ptr<net>> sortedNets{};

    while (highestSinkBlockCount > 0)
    {
        auto range = netsGroupedBySinkBlockCount.equal_range(highestSinkBlockCount);
        for (auto it = range.first; it != range.second; it++)
            sortedNets.push_back(std::shared_ptr<net>(it->second));

        highestSinkBlockCount--;
    }

    return sortedNets;
}

void deepCopy(std::vector<std::shared_ptr<net>> sortedNets, std::vector<std::shared_ptr<net>> &copyOfSortedNets, std::map<std::string, std::shared_ptr<block>> const &blocks, std::map<std::string, std::shared_ptr<block>> &copyOfBlocks)
{
    copyOfSortedNets = std::vector<std::shared_ptr<net>>{};
    copyOfBlocks = std::map<std::string, std::shared_ptr<block>>{};

    for (std::shared_ptr<net> p_net : sortedNets)
        copyOfSortedNets.push_back(std::make_shared<net>(*p_net));

    for (auto &entry : blocks)
        copyOfBlocks.insert(std::make_pair(entry.first, std::make_shared<block>(*entry.second)));

    /*     sortedNets[0]->setUsedTrack(channelID{0,2,constants::channelTypeY}, 2);
        sortedNets[0]->setSourceBlockName("ef");

        blocks.find("c1")->second->setChannelTaken(channelID{2,1,constants::channelTypeY});
        blocks.find("c1")->second->initialise(3,2,0);
        std::cout<<"fhi"; */
}

int main(int argc, char *argv[])
{

    abortIfTrue(argc != 2, constants::wrongArguments, "Argument count unexpected! Arguments received: '" + argsToString(argc, argv) + " Please enter just one argument: the filename for the .net and .place files to be used.");

    std::cout << "Program start!\n";

    unsigned char arraySize{};
    std::string clockName{};
    std::map<std::string, std::shared_ptr<block>> blocks{};
    std::map<std::string, std::shared_ptr<net>> netsByNameOfTheSourceBlock{};
    std::map<std::string, std::shared_ptr<net>> netsByNameOfTheNet{};
    std::map<std::string, std::pair<unsigned short, std::shared_ptr<block>>> blocksConnectedToClock{};

    std::string fileName = argv[1];
    std::string errorMessage{};
    bool success{};

    success = readNet(fileName, netsByNameOfTheSourceBlock, netsByNameOfTheNet, blocks, errorMessage);
    abortIfTrue(!success, constants::invalidNetFile, errorMessage);

    success = readPlace(fileName, arraySize, netsByNameOfTheSourceBlock, blocks, blocksConnectedToClock, errorMessage);
    abortIfTrue(!success, constants::invalidPlaceFile, errorMessage);

    assert(netsByNameOfTheNet.size() == netsByNameOfTheSourceBlock.size());

    std::vector<std::shared_ptr<net>> sortedNets = sortNets(netsByNameOfTheSourceBlock);

    unsigned char channelwidth = constants::startingValueChannelWidth;

    std::cout << "The .net and .place files have been successfully read!\n"
              << "The arraySize is " << +arraySize << ".\n"
              << sortedNets.size() << " nets and " << blocks.size() << " blocks have been read.\n";
    /*
    << "blocksConnectedToClock: " << blocksConnectedToClock.size() << '\n'
    << "name of the clock: " << clockName << '\n'; */

    unsigned char successfulWidth = std::numeric_limits<unsigned char>::max();
    unsigned char failedWidth = 0;
    std::vector<std::shared_ptr<net>> tempSortedNets{}, finalSortedNets{};
    std::map<std::string, std::shared_ptr<block>> tempBlocks{}, finalBlocks{};

    do
    {
        std::cout << "\n----------------\nRouting nets with a channelwidth of " << +channelwidth << " tracks!\n";
        deepCopy(sortedNets, tempSortedNets, blocks, tempBlocks);
        success = routeNets(arraySize, channelwidth, tempSortedNets, tempBlocks);

        if (success)
        {
            successfulWidth = channelwidth;
            finalSortedNets = tempSortedNets;
            finalBlocks = tempBlocks;
            std::cout << "Routing succeeded with a channelwidth of " << +successfulWidth << " tracks!\n";
            channelwidth = channelwidth / 2;
            if (channelwidth <= failedWidth)
                channelwidth = failedWidth + 1;
            assert(channelwidth > 0);
        }
        else
        {
            assert(channelwidth > failedWidth);
            failedWidth = channelwidth;
            std::cout << "Routing failed with a channelwidth of " << +failedWidth << " tracks!\n";
            if (successfulWidth == channelwidth + 2)
                channelwidth += 1;
            else
                channelwidth += 2;
        }

        assert(channelwidth <= constants::maximumChannelWidth + 1);
    } while (channelwidth < successfulWidth);
    std::cout << "Using the result from the successful run with a channelwidth of " << +successfulWidth << " tracks!\n";

    std::cout << "Writing routing file!\n";
    writeRouting(fileName, arraySize, finalSortedNets, finalBlocks);

    return constants::success;
}
