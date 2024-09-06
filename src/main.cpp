#include <chrono>
#include <iostream>
#include <regex>
#include <cassert>
#include <thread>
#include "constants.hpp"
#include "io.hpp"
#include "channel/channel.hpp"
#include "routing.hpp"
#include "block.hpp"
#include "logging.hpp"

void abortIfTrue(bool condition, unsigned char errorCode, std::string const &errorMessage)
{
    if (condition)
    {
        std::cerr << errorMessage;
        std::flush(std::cerr);
        exit(errorCode);
    }
}

void sortNets(std::map<std::string, std::shared_ptr<net>> const &netsByNameOfTheNet, std::set<std::shared_ptr<net>> const &globalNets, std::vector<std::shared_ptr<net>> &sortedNets,
              std::vector<std::shared_ptr<net>> &unsortedNets)
{
    std::multimap<unsigned short, std::shared_ptr<net>> netsGroupedByConnectedBlockCount{};
    unsigned short highestConnectedBlockCount = 0;

    unsortedNets.reserve(netsByNameOfTheNet.size() - globalNets.size());

    for (auto const &entry : netsByNameOfTheNet)
    {
        std::shared_ptr<net> p_net = entry.second;
        if (!globalNets.contains(p_net))
        {
            unsortedNets.push_back(p_net);

            netsGroupedByConnectedBlockCount.insert(std::make_pair(p_net->getConnectedBlockCount(), p_net));
            if (highestConnectedBlockCount < p_net->getConnectedBlockCount())
                highestConnectedBlockCount = p_net->getConnectedBlockCount();
        }
    }

    sortedNets.reserve(netsByNameOfTheNet.size() - globalNets.size());

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

    success = readNet(netFileName, netsByNameOfTheSourceBlock, netsByNameOfTheNet, globalNets, blocks, errorMessage);
    abortIfTrue(!success, constants::invalidNetFile, errorMessage);

    success = readPlace(placeFileName, arraySize, netsByNameOfTheSourceBlock, blocks, errorMessage);
    abortIfTrue(!success, constants::invalidPlaceFile, errorMessage);

    assert(netsByNameOfTheNet.size() == netsByNameOfTheSourceBlock.size());

    sortNets(netsByNameOfTheNet, globalNets, sortedNets, unsortedNets);
}

void deepCopy(std::vector<std::shared_ptr<net>> const &sortedNets, std::vector<std::shared_ptr<net>> &copyOfSortedNets, std::map<std::string, std::shared_ptr<block>> const &blocks,
              std::map<std::string, std::shared_ptr<block>> &copyOfBlocks)
{
    copyOfSortedNets = std::vector<std::shared_ptr<net>>{};
    copyOfBlocks = std::map<std::string, std::shared_ptr<block>>{};

    for (std::shared_ptr<net> const &p_net : sortedNets)
        copyOfSortedNets.push_back(std::make_shared<net>(*p_net));

    for (auto &entry : blocks)
        copyOfBlocks.insert(std::make_pair(entry.first, std::make_shared<block>(*entry.second)));
}

void copyAndRoute(std::vector<std::shared_ptr<net>> const &nets, std::vector<std::shared_ptr<net>> &tempNets, std::map<std::string, std::shared_ptr<block>> const &blocks,
                  std::map<std::string, std::shared_ptr<block>> &tempBlocks, unsigned char arraySize, unsigned char channelWidth, std::size_t &netsRouted)
{
    deepCopy(nets, tempNets, blocks, tempBlocks);
    netsRouted = routeNets(arraySize, channelWidth, tempNets, tempBlocks);
}

void tryRoutingWithChannelWidth(std::vector<std::shared_ptr<net>> const &sortedNets, std::vector<std::shared_ptr<net>> const &unsortedNets, std::vector<std::shared_ptr<net>> &finalNets,
                                std::map<std::string, std::shared_ptr<block>> const &blocks, std::map<std::string, std::shared_ptr<block>> &finalBlocks, unsigned char arraySize,
                                unsigned char channelWidth, bool &routingIsSorted, std::size_t &netsRouted)
{
    printLogMessage("Trying routing with a channel width of " + std::to_string(channelWidth));

    size_t netsRoutedSorted{};
    std::vector<std::shared_ptr<net>> tempNetsSorted;
    std::map<std::string, std::shared_ptr<block>> tempBlocksSorted;

    std::thread sortedThread(copyAndRoute, std::cref(sortedNets), std::ref(tempNetsSorted), std::cref(blocks), std::ref(tempBlocksSorted), arraySize, channelWidth, std::ref(netsRoutedSorted));

    size_t netsRoutedUnsorted{};
    std::vector<std::shared_ptr<net>> tempNetsUnsorted;
    std::map<std::string, std::shared_ptr<block>> tempBlocksUnsorted;

    std::thread unsortedThread(copyAndRoute, std::cref(unsortedNets), std::ref(tempNetsUnsorted), std::cref(blocks), std::ref(tempBlocksUnsorted), arraySize, channelWidth, std::ref(netsRoutedUnsorted));

    sortedThread.join();
    unsortedThread.join();

    if (netsRoutedSorted == sortedNets.size())
    {
        routingIsSorted = true;
        netsRouted = netsRoutedSorted;
        finalNets = std::move(tempNetsSorted);
        finalBlocks = std::move(tempBlocksSorted);
    }
    else if (netsRoutedUnsorted == sortedNets.size())
    {
        routingIsSorted = false;
        netsRouted = netsRoutedUnsorted;
        finalNets = std::move(tempNetsUnsorted);
        finalBlocks = std::move(tempBlocksUnsorted);
    }
    else
        netsRouted = netsRoutedSorted;
}

void routeAndMinimiseChannelWidth(std::vector<std::shared_ptr<net>> const &sortedNets, std::vector<std::shared_ptr<net>> const &unsortedNets, std::vector<std::shared_ptr<net>> &finalNets,
                                  std::map<std::string, std::shared_ptr<block>> const &blocks, std::map<std::string, std::shared_ptr<block>> &finalBlocks, unsigned char arraySize,
                                  bool &routingIsSorted)
{
    unsigned char channelWidth = constants::startingValueChannelWidth;
    std::size_t netsRouted{};
    unsigned char successfulWidth = std::numeric_limits<unsigned char>::max();
    unsigned char failedWidth = 0;
    std::vector<std::shared_ptr<net>> tempNets{};
    std::map<std::string, std::shared_ptr<block>> tempBlocks{};

    while (failedWidth < successfulWidth - 1)
    {
        tryRoutingWithChannelWidth(sortedNets, unsortedNets, finalNets, blocks, finalBlocks, arraySize, channelWidth, routingIsSorted, netsRouted);

        if (netsRouted == sortedNets.size())
        {
            successfulWidth = channelWidth;
            printLogMessage("Routing succeeded with a channelWidth of " + std::to_string(+successfulWidth) + " tracks!");

            if (failedWidth != 0)
                channelWidth = (channelWidth + failedWidth) / 2;
            else
                channelWidth = successfulWidth > 1 ? channelWidth / 2 : successfulWidth;
        }
        else
        {
            assert(channelWidth > failedWidth);
            failedWidth = channelWidth;
            printLogMessage("Routing failed with a channelWidth of " + std::to_string(+failedWidth) + " tracks!");

            if (netsRouted * constants::ratioEarlyFail < sortedNets.size())
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
    }
    printLogMessage("Using the result from the successful run with a channelWidth of " + std::to_string(+successfulWidth) + " tracks!");
}

int main(int argc, char *argv[])
{
    auto startTime = std::chrono::steady_clock::now();

    abortIfTrue(argc != 4 && argc != 5, constants::wrongArguments, "Argument count unexpected! Arguments received: '" + argsToString(argc, argv) + constants::correctArgumentsMessage);
    abortIfTrue(argc == 5 && !std::regex_match(argv[4], constants::numberPattern), constants::wrongArguments, "The fourth argument was not a number! Arguments received: '" + argsToString(argc, argv) + constants::correctArgumentsMessage);

    unsigned char channelWidthToTry = std::numeric_limits<unsigned char>::max();
    if (argc == 5)
        channelWidthToTry = std::stoi(argv[4]);

    unsigned char arraySize{};
    std::string netFileName = argv[1];
    std::string placeFileName = argv[2];
    std::string routeFileName = argv[3];
    std::map<std::string, std::shared_ptr<block>> blocks;
    std::set<std::shared_ptr<net>> globalNets;

    std::vector<std::shared_ptr<net>> sortedNets, unsortedNets;
    readNetsAndBlocks(netFileName, placeFileName, arraySize, blocks, globalNets, sortedNets, unsortedNets);

    std::string logMessage = "The .net and .place files have been successfully read!\n" + std::to_string(sortedNets.size() + globalNets.size()) + " nets ";
    if (globalNets.size() > 0)
        logMessage += '(' + std::to_string(globalNets.size()) + " global nets) ";
    logMessage += "and " + std::to_string(blocks.size()) + " blocks have been read.\n";
    printLogMessage(logMessage);

    bool routingIsSorted{};
    std::vector<std::shared_ptr<net>> finalNets;
    std::map<std::string, std::shared_ptr<block>> finalBlocks;

    if (channelWidthToTry != std::numeric_limits<unsigned char>::max())
    {
        std::size_t netsRouted{};
        tryRoutingWithChannelWidth(sortedNets, unsortedNets, finalNets, blocks, finalBlocks, arraySize, channelWidthToTry, routingIsSorted, netsRouted);

        if (netsRouted == sortedNets.size())
        {
            printLogMessage("Routing succeeded with the given channel width of " + std::to_string(channelWidthToTry) + '.');
            if (!routingIsSorted)
                printLogMessage("Routing failed with pre-sorting, but succeeded without sorting, so the result of the unsorted routing is used.");
        }
        else
        {
            printLogMessage("Routing failed with the given channel width of " + std::to_string(channelWidthToTry) + '.');
            return constants::channelWidthTooLow;
        }
    }
    else
    {
        routeAndMinimiseChannelWidth(sortedNets, unsortedNets, finalNets, blocks, finalBlocks, arraySize, routingIsSorted);

        if (!routingIsSorted)
            printLogMessage("Routing with pre-sorting (for minimizing delay) would have required a higher channelwidh, so the result of the unsorted routing is used instead.");
    }

    printLogMessage("Writing routing file!");
    writeRouting(routeFileName, arraySize, finalNets, globalNets, finalBlocks);

    auto endTime = std::chrono::steady_clock::now();

    std::chrono::duration<double> duration{endTime - startTime};
    std::cout << "The program ran for " << duration << ".\n";

    return constants::success;
}
