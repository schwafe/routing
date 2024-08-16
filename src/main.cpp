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

std::vector<std::shared_ptr<net>> sortNets(std::map<std::string, std::shared_ptr<net>> const &netsByNameOfTheNet, std::set<std::string> const &netsConnectedToClock,
                                           std::set<std::shared_ptr<net>> const &globalNets)
{
    std::multimap<unsigned short, std::shared_ptr<net>> netsGroupedBySinkBlockCount{};
    std::multimap<unsigned short, std::shared_ptr<net>> netsConnectedToClockGroupedBySinkBlockCount{};
    unsigned short highestSinkBlockCountClock = 0;
    unsigned short highestSinkBlockCount = 0;

    for (auto entry : netsByNameOfTheNet)
    {
        std::shared_ptr<net> p_net = entry.second;
        if (!globalNets.contains(p_net))
        {
            if (netsConnectedToClock.contains(entry.first))
            {
                unsigned short sinkBlockCount = p_net->getSinkBlockCount();
                netsConnectedToClockGroupedBySinkBlockCount.insert(std::make_pair(sinkBlockCount, p_net));

                if (highestSinkBlockCountClock < sinkBlockCount)
                    highestSinkBlockCountClock = sinkBlockCount;
            }
            else
            {
                netsGroupedBySinkBlockCount.insert(std::make_pair(p_net->getSinkBlockCount(), p_net));
                if (highestSinkBlockCount < p_net->getSinkBlockCount())
                    highestSinkBlockCount = p_net->getSinkBlockCount();
            }
        }
    }

    assert(!netsGroupedBySinkBlockCount.contains(0));

    std::vector<std::shared_ptr<net>> sortedNets{};
    sortedNets.reserve(netsByNameOfTheNet.size());

    while (highestSinkBlockCountClock > 0)
    {
        auto range = netsConnectedToClockGroupedBySinkBlockCount.equal_range(highestSinkBlockCountClock);
        for (auto it = range.first; it != range.second; it++)
            sortedNets.push_back(std::shared_ptr<net>(it->second));

        highestSinkBlockCountClock--;
    }

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
}

int main(int argc, char *argv[])
{

    abortIfTrue(argc != 2, constants::wrongArguments, "Argument count unexpected! Arguments received: '" + argsToString(argc, argv) + " Please enter just one argument: the filename for the .net and .place files to be used.");

    printLogMessage("Program start!");

    unsigned char arraySize{};
    std::map<std::string, std::shared_ptr<block>> blocks{};
    std::map<std::string, std::shared_ptr<net>> netsByNameOfTheSourceBlock{};
    std::map<std::string, std::shared_ptr<net>> netsByNameOfTheNet{};
    std::set<std::string> netsConnectedToClock{};
    std::set<std::shared_ptr<net>> globalNets{};

    std::string fileName = argv[1];
    std::string errorMessage{};
    bool success{};

    success = readNet(fileName, netsByNameOfTheSourceBlock, netsByNameOfTheNet, netsConnectedToClock, globalNets, blocks, errorMessage);
    abortIfTrue(!success, constants::invalidNetFile, errorMessage);

    success = readPlace(fileName, arraySize, netsByNameOfTheSourceBlock, blocks, errorMessage);
    abortIfTrue(!success, constants::invalidPlaceFile, errorMessage);

    assert(netsByNameOfTheNet.size() == netsByNameOfTheSourceBlock.size());

    std::vector<std::shared_ptr<net>> sortedNets = sortNets(netsByNameOfTheNet, netsConnectedToClock, globalNets);

    unsigned char channelwidth = constants::startingValueChannelWidth;

    std::string logMessage = "The .net and .place files have been successfully read!\nThe arraySize is " + std::to_string(+arraySize) + ".\n" + std::to_string(sortedNets.size() + globalNets.size()) + " nets ";
    if (globalNets.size() > 0)
        logMessage += '(' + std::to_string(globalNets.size()) + " global nets) ";
    logMessage += "and " + std::to_string(blocks.size()) + " blocks have been read.\n";
    printLogMessage(logMessage);

    unsigned char successfulWidth = std::numeric_limits<unsigned char>::max();
    unsigned char failedWidth = 0;
    std::vector<std::shared_ptr<net>> tempSortedNets{}, finalSortedNets{};
    std::map<std::string, std::shared_ptr<block>> tempBlocks{}, finalBlocks{};

    do
    {
        printLogMessage("\n----------------\nRouting nets with a channelwidth of " + std::to_string(+channelwidth) + " tracks!");
        deepCopy(sortedNets, tempSortedNets, blocks, tempBlocks);
        success = routeNets(arraySize, channelwidth, tempSortedNets, tempBlocks);

        if (success)
        {
            successfulWidth = channelwidth;
            finalSortedNets = tempSortedNets;
            finalBlocks = tempBlocks;
            printLogMessage("Routing succeeded with a channelwidth of " + std::to_string(+successfulWidth) + " tracks!");
            channelwidth = channelwidth / 2;
            if (channelwidth <= failedWidth)
                channelwidth = failedWidth + 1;
            assert(channelwidth > 0);
        }
        else
        {
            assert(channelwidth > failedWidth);
            failedWidth = channelwidth;
            printLogMessage("Routing failed with a channelwidth of " + std::to_string(+failedWidth) + " tracks!");
            if (successfulWidth == channelwidth + 2)
                channelwidth += 1;
            else
                channelwidth += 2;
        }

        assert(channelwidth <= constants::maximumChannelWidth + 1);
    } while (channelwidth < successfulWidth);
    printLogMessage("Using the result from the successful run with a channelwidth of " + std::to_string(+successfulWidth) + " tracks!");

    printLogMessage("Writing routing file!");
    writeRouting(fileName, arraySize, finalSortedNets, globalNets, finalBlocks);

    return constants::success;
}
