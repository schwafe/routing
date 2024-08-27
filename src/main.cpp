#include <time.h>
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

void sortNets(std::map<std::string, std::shared_ptr<net>> const &netsByNameOfTheNet, std::set<std::string> const &netsConnectedToClock,
              std::set<std::shared_ptr<net>> const &globalNets, std::vector<std::shared_ptr<net>> &sortedNets, std::vector<std::shared_ptr<net>> &unsortedNets)
{
    std::multimap<unsigned short, std::shared_ptr<net>> netsGroupedByConnectedBlockCount{};
    std::multimap<unsigned short, std::shared_ptr<net>> netsConnectedToClockGroupedByConnectedBlockCount{};
    unsigned short highestConnectedBlockCountClock = 0;
    unsigned short highestConnectedBlockCount = 0;

    unsortedNets.reserve(netsByNameOfTheNet.size() - globalNets.size());

    for (auto entry : netsByNameOfTheNet)
    {
        std::shared_ptr<net> p_net = entry.second;
        if (!globalNets.contains(p_net))
        {
            unsortedNets.push_back(p_net);

            if (netsConnectedToClock.contains(entry.first))
            {
                unsigned short connectedBlockCount = p_net->getConnectedBlockCount();
                netsConnectedToClockGroupedByConnectedBlockCount.insert(std::make_pair(connectedBlockCount, p_net));

                if (highestConnectedBlockCountClock < connectedBlockCount)
                    highestConnectedBlockCountClock = connectedBlockCount;
            }
            else
            {
                netsGroupedByConnectedBlockCount.insert(std::make_pair(p_net->getConnectedBlockCount(), p_net));
                if (highestConnectedBlockCount < p_net->getConnectedBlockCount())
                    highestConnectedBlockCount = p_net->getConnectedBlockCount();
            }
        }
    }

    assert(!netsGroupedByConnectedBlockCount.contains(0));

    sortedNets.reserve(netsByNameOfTheNet.size() - globalNets.size());

    while (highestConnectedBlockCountClock > 0)
    {
        auto range = netsConnectedToClockGroupedByConnectedBlockCount.equal_range(highestConnectedBlockCountClock);
        for (auto it = range.first; it != range.second; it++)
            sortedNets.push_back(it->second);

        highestConnectedBlockCountClock--;
    }

    while (highestConnectedBlockCount > 0)
    {
        auto range = netsGroupedByConnectedBlockCount.equal_range(highestConnectedBlockCount);
        for (auto it = range.first; it != range.second; it++)
            sortedNets.push_back(it->second);

        highestConnectedBlockCount--;
    }
}

void readNetsAndBlocks(std::string const &netFileName, std::string const &placeFileName, unsigned char &arraySize,
                       std::map<std::string, std::shared_ptr<block>> &blocks, std::set<std::shared_ptr<net>> &globalNets,
                       std::vector<std::shared_ptr<net>> &sortedNets, std::vector<std::shared_ptr<net>> &unsortedNets)
{
    std::string errorMessage{};
    bool success{};
    std::map<std::string, std::shared_ptr<net>> netsByNameOfTheSourceBlock{};
    std::map<std::string, std::shared_ptr<net>> netsByNameOfTheNet{};
    std::set<std::string> netsConnectedToClock{};

    success = readNet(netFileName, netsByNameOfTheSourceBlock, netsByNameOfTheNet, netsConnectedToClock, globalNets, blocks, errorMessage);
    abortIfTrue(!success, constants::invalidNetFile, errorMessage);

    success = readPlace(placeFileName, arraySize, netsByNameOfTheSourceBlock, blocks, errorMessage);
    abortIfTrue(!success, constants::invalidPlaceFile, errorMessage);

    assert(netsByNameOfTheNet.size() == netsByNameOfTheSourceBlock.size());

    sortNets(netsByNameOfTheNet, netsConnectedToClock, globalNets, sortedNets, unsortedNets);
}

void deepCopy(std::vector<std::shared_ptr<net>> const &sortedNets, std::vector<std::shared_ptr<net>> &copyOfSortedNets, std::map<std::string, std::shared_ptr<block>> const &blocks,
              std::map<std::string, std::shared_ptr<block>> &copyOfBlocks)
{
    copyOfSortedNets = std::vector<std::shared_ptr<net>>{};
    copyOfBlocks = std::map<std::string, std::shared_ptr<block>>{};

    for (std::shared_ptr<net> p_net : sortedNets)
        copyOfSortedNets.push_back(std::make_shared<net>(*p_net));

    for (auto &entry : blocks)
        copyOfBlocks.insert(std::make_pair(entry.first, std::make_shared<block>(*entry.second)));
}

void tryRoutingWithChannelWidth(std::vector<std::shared_ptr<net>> const &sortedNets, std::vector<std::shared_ptr<net>> const &unsortedNets, std::vector<std::shared_ptr<net>> &tempNets,
                                std::map<std::string, std::shared_ptr<block>> const &blocks, std::map<std::string, std::shared_ptr<block>> &tempBlocks, unsigned char arraySize,
                                unsigned char channelWidth, bool &routingIsSorted, unsigned short &netsRouted)
{
    deepCopy(sortedNets, tempNets, blocks, tempBlocks);
    netsRouted = routeNets(arraySize, channelWidth, tempNets, tempBlocks);

    if (netsRouted == sortedNets.size())
        routingIsSorted = true;
    else
    {
        deepCopy(unsortedNets, tempNets, blocks, tempBlocks);
        netsRouted = routeNets(arraySize, channelWidth, tempNets, tempBlocks);
        if (netsRouted == sortedNets.size())
            routingIsSorted = false;
    }
}

void routeAndMinimiseChannelWidth(std::vector<std::shared_ptr<net>> const &sortedNets, std::vector<std::shared_ptr<net>> const &unsortedNets, std::vector<std::shared_ptr<net>> &finalNets,
                                  std::map<std::string, std::shared_ptr<block>> const &blocks, std::map<std::string, std::shared_ptr<block>> &finalBlocks, unsigned char arraySize,
                                  unsigned char channelWidth, bool &routingIsSorted)
{
    unsigned short netsRouted{};
    unsigned char successfulWidth = std::numeric_limits<unsigned char>::max();
    unsigned char failedWidth = 0;
    std::vector<std::shared_ptr<net>> tempNets{};
    std::map<std::string, std::shared_ptr<block>> tempBlocks{};
    do
    {
        tryRoutingWithChannelWidth(sortedNets, unsortedNets, tempNets, blocks, tempBlocks, arraySize, channelWidth, routingIsSorted, netsRouted);

        if (netsRouted == sortedNets.size())
        {
            successfulWidth = channelWidth;
            finalNets = tempNets;
            finalBlocks = tempBlocks;
            printLogMessage("Routing succeeded with a channelWidth of " + std::to_string(+successfulWidth) + " tracks!");

            if (failedWidth != 0)
                channelWidth = (channelWidth + failedWidth) / 2;
            else
                channelWidth = channelWidth / 2;

            assert(channelWidth > 0);
        }
        else
        {
            assert(channelWidth > failedWidth);
            failedWidth = channelWidth;
            printLogMessage("Routing failed with a channelWidth of " + std::to_string(+failedWidth) + " tracks!");

            if (netsRouted * 1.5 < sortedNets.size())
            {
                if (channelWidth + 4 < successfulWidth)
                    channelWidth += 4;
                else
                    channelWidth = successfulWidth - 1;
            }
            else
            {
                if (channelWidth + 2 < successfulWidth)
                    channelWidth += 2;
                else
                    channelWidth = successfulWidth - 1;
            }
        }

        assert(channelWidth <= constants::maximumChannelWidth + 1);
    } while (failedWidth < successfulWidth - 1);
    printLogMessage("Using the result from the successful run with a channelWidth of " + std::to_string(+successfulWidth) + " tracks!");
}

int main(int argc, char *argv[])
{
    clock_t startTime = clock();

    abortIfTrue(argc != 4 && argc != 5, constants::wrongArguments, "Argument count unexpected! Arguments received: '" + argsToString(argc, argv) + constants::correctArgumentsMessage);
    abortIfTrue(!std::regex_match(argv[4], constants::numberPattern), constants::wrongArguments, "The fourth argument was not a number! Arguments received: '" + argsToString(argc, argv) + constants::correctArgumentsMessage);

    unsigned char channelWidthToTry = std::numeric_limits<unsigned char>::max();
    if (argc == 5)
        channelWidthToTry = std::stoi(argv[4]);

    unsigned char arraySize{};
    std::string netFileName = argv[1];
    std::string placeFileName = argv[2];
    std::string routeFileName = argv[3];
    std::map<std::string, std::shared_ptr<block>> blocks{};
    std::set<std::shared_ptr<net>> globalNets{};

    unsigned char channelWidth = constants::startingValueChannelWidth;

    std::vector<std::shared_ptr<net>> sortedNets{}, unsortedNets{};
    readNetsAndBlocks(netFileName, placeFileName, arraySize, blocks, globalNets, sortedNets, unsortedNets);

    std::string logMessage = "The .net and .place files have been successfully read!\n" + std::to_string(sortedNets.size() + globalNets.size()) + " nets ";
    if (globalNets.size() > 0)
        logMessage += '(' + std::to_string(globalNets.size()) + " global nets) ";
    logMessage += "and " + std::to_string(blocks.size()) + " blocks have been read.\n";
    printLogMessage(logMessage);

    bool routingIsSorted{};
    std::vector<std::shared_ptr<net>> finalNets{};
    std::map<std::string, std::shared_ptr<block>> finalBlocks{};

    if (channelWidthToTry != std::numeric_limits<unsigned char>::max())
    {

        std::vector<std::shared_ptr<net>> tempNets{};
        std::map<std::string, std::shared_ptr<block>> tempBlocks{};
        unsigned short netsRouted{};
        tryRoutingWithChannelWidth(sortedNets, unsortedNets, tempNets, blocks, tempBlocks, arraySize, channelWidth, routingIsSorted, netsRouted);

        if (netsRouted == sortedNets.size())
        {
            printLogMessage("Routing succeeded with the given channel width of " + std::to_string(channelWidth) + '.');
            if (!routingIsSorted)
                printLogMessage("Routing failed with pre-sorting, but succeeded without sorting, so the result of the unsorted routing is used.");
        }
        else
            printLogMessage("Routing failed with the given channel width of " + std::to_string(channelWidth) + '.');
    }
    else
        routeAndMinimiseChannelWidth(sortedNets, unsortedNets, finalNets, blocks, finalBlocks, arraySize, channelWidth, routingIsSorted);

    if (!routingIsSorted)
        printLogMessage("Routing with pre-sorting (for minimizing delay) would have required a higher channelwidh, so the result of the unsorted routing is used instead.");

    printLogMessage("Writing routing file!");
    writeRouting(routeFileName, arraySize, finalNets, globalNets, finalBlocks);

    clock_t endTime = clock();

    double duration = 1.0 * (endTime - startTime) / CLOCKS_PER_SEC;

    printf("Program ran for: %.3f seconds.\n", duration);

    return constants::success;
}
