#include <iostream>
#include <cassert>
#include "routing.hpp"
#include "block.hpp"
#include "logging.hpp"
#include "constants.hpp"

std::map<channelID, std::set<std::string>> generateRelevantChannels(std::shared_ptr<net> p_net, std::map<std::string, std::shared_ptr<block>> &blocks)
{
    std::map<channelID, std::set<std::string>> relevantChannels;
    for (auto &connectedPinEntry : p_net->getConnectedPinBlockNamesAndTheirRouting())
    {
        std::string sinkBlockName = connectedPinEntry.first;

        assert(blocks.contains(sinkBlockName));
        for (channelID channel : blocks.find(sinkBlockName)->second->getOpenChannels())
        {
            if (relevantChannels.contains(channel))
            {
                relevantChannels.find(channel)->second.insert(sinkBlockName);
            }
            else
            {
                std::set<std::string> associatedBlocks{sinkBlockName};
                relevantChannels.insert(std::make_pair(channel, associatedBlocks));
            }
        }
    }
    return relevantChannels;
}

void removeOtherChannelEntries(std::map<channelID, std::set<std::string>> &relevantChannels, const std::string &blockName, const std::set<channelID> &openChannels)
{
    for (channelID channel : openChannels)
    {
        assert(relevantChannels.contains(channel));
        std::set<std::string> &associatedBlocks = relevantChannels.find(channel)->second;
        associatedBlocks.erase(blockName);
        if (associatedBlocks.empty())
            relevantChannels.erase(channel);
    }
}

void registerIndex(std::map<channelID, unsigned char> &channelToIndex, std::map<unsigned char, std::set<channelID>> &indexToChannels, const channelID &channel, const unsigned char &index)
{
    assert(!channelToIndex.contains(channel) || channelToIndex.find(channel)->second > index || (channelToIndex.find(channel)->second == constants::indexZero && index == constants::indexZero));
    channelToIndex.insert_or_assign(channel, index);
    if (!indexToChannels.contains(index))
        indexToChannels.emplace(index, std::set<channelID>{});
    indexToChannels.find(index)->second.emplace(channel);
}

/* bool addChannelIfValid(channelID channel, auto &channelToIndex, auto &indexToChannels, unsigned char index, unsigned char const &channelwidth, auto &channelInformation, auto &newWave,
                       auto &relevantChannels, unsigned short &numberOfPinsReached, channelID &sink, std::set<std::string> &reachedBlocks, auto &blocks)
{
    assert(channelInformation.contains(channel));
    if (!channelToIndex.contains(channel) && !channelInformation.find(channel)->second.isFull(channelwidth))
    {
        registerIndex(channelToIndex, indexToChannels, channel, index);

        if (relevantChannels.contains(channel))
        {
            assert(relevantChannels.find(channel)->second.size() != 0);
            for (std::string blockName : relevantChannels.find(channel)->second)
            {
                numberOfPinsReached++;
                reachedBlocks.insert(blockName);
                assert(blocks.contains(blockName));
                std::shared_ptr<block> p_block = blocks.find(blockName)->second;
                p_block->setChannelTaken(channel);
                removeOtherChannelEntries(relevantChannels, blockName, p_block->getOpenChannels());
            }
            relevantChannels.erase(channel);
            sink = channel;

            return true;
        }
        else
        {
            newWave.insert(channel);
        }
    }
    return false;
}

unsigned char wavePropagation(unsigned char const &arraySize, auto &channelToIndex, auto &indexToChannels, unsigned char const &channelwidth, auto &channelInformation,
                              auto &relevantChannels, unsigned short &numberOfPinsReached, channelID &sink, std::set<std::string> &reachedBlocks, auto &blocks)
{
    unsigned char currentIndex = 1;

    assert(indexToChannels.contains(constants::indexZero));
    std::set<channelID> currentWave(indexToChannels.find(constants::indexZero)->second);
    std::set<channelID> newWave;
    do
    {
        for (channelID channel : currentWave)
        {
            for (channelID neighbour : channel.getNeighbours(arraySize))
            {
                bool pinFound = addChannelIfValid(neighbour, channelToIndex, indexToChannels, currentIndex, channelwidth, channelInformation, newWave, relevantChannels, numberOfPinsReached, sink, reachedBlocks, blocks);
                if (pinFound)
                    return currentIndex;
            }
        }
        currentIndex++;

        currentWave.clear();
        currentWave = newWave;
        newWave.clear();
    } while (!currentWave.empty());

    return -1;
}
 */

channelID findSink(std::map<channelID, unsigned char> &channelToIndex, std::map<unsigned char, std::set<channelID>> &indexToChannels, const unsigned char &arraySize, unsigned char &indexOfSink,
                   const unsigned char &channelwidth, auto &channelInformation, auto &relevantChannels, unsigned short &numberOfPinsReached, std::set<std::string> &reachedBlocks, const auto &blocks)
{
    // TODO test

    std::set<channelID> processedChannels{};
    unsigned char index = constants::indexZero;
    assert(indexToChannels.contains(constants::indexZero));
    do
    {
        unsigned char expectedIndex = index + 1;

        std::set<channelID> channels = indexToChannels.find(index)->second;
        for (channelID channel : channels)
        {
            if (!processedChannels.contains(channel))
            {
                processedChannels.emplace(channel);

                for (channelID neighbour : channel.getNeighbours(arraySize))
                {
                    assert(channelInformation.contains(neighbour));
                    if (channelToIndex.contains(neighbour))
                    {
                        assert(channelToIndex.find(neighbour)->second >= index - 1);
                        if (channelToIndex.find(neighbour)->second > expectedIndex)
                        {
                            channelToIndex.insert_or_assign(neighbour, expectedIndex);

                            if (!indexToChannels.contains(expectedIndex))
                                indexToChannels.emplace(expectedIndex, std::set<channelID>{});
                            indexToChannels.find(expectedIndex)->second.emplace(neighbour);
                        }
                    }
                    else if (!channelInformation.find(neighbour)->second.isFull(channelwidth))
                    {
                        registerIndex(channelToIndex, indexToChannels, neighbour, expectedIndex);

                        if (relevantChannels.contains(neighbour))
                        {
                            assert(relevantChannels.find(neighbour)->second.size() != 0);
                            for (std::string blockName : relevantChannels.find(neighbour)->second)
                            {
                                numberOfPinsReached++;
                                reachedBlocks.insert(blockName);
                                assert(blocks.contains(blockName));
                                std::shared_ptr<block> p_block = blocks.find(blockName)->second;
                                p_block->setChannelTaken(neighbour);
                                removeOtherChannelEntries(relevantChannels, blockName, p_block->getOpenChannels());
                            }
                            relevantChannels.erase(neighbour);

                            indexOfSink = expectedIndex;
                            return neighbour;
                        }
                    }
                }
            }
        }
    } while (indexToChannels.contains(++index));

    return channelID{};
}

void useChannel(const channelID &channel, auto &channelInformation, std::stack<std::pair<channelID, unsigned char>> &connectionToSink, std::set<channelID> &usedChannels)
{
    assert(channelInformation.contains(channel));
    unsigned char track = channelInformation.find(channel)->second.useChannel();
    connectionToSink.emplace(channel, track);
    usedChannels.insert(channel);
}

std::stack<std::pair<channelID, unsigned char>> retrace(unsigned char const &arraySize, auto &channelToIndex, auto &indexToChannels, channelID sink, unsigned char &indexOfSink, auto &channelInformation)
{
    assert(sink.getType() != 0);
    std::set<channelID> usedChannels{};
    std::stack<std::pair<channelID, unsigned char>> connectionToSink{};
    useChannel(sink, channelInformation, connectionToSink, usedChannels);

    std::cout << "-------------\nsink: " << channelIDToString(sink) << std::endl;

    // printIndices(channelToIndex);

    channelID currentChannel = sink;
    unsigned char expectedIndex = indexOfSink;
    // TODO most likely using channels at index 0 more than one time. Connection to sink (and the retrace step) need to stop before a channel is used twice. For the first connection, this is no problem, but all the following ones need to stop at index 1.
    // not quite correct? connectionToSink does need the index 0 element, but if the track is the same, the channel does not need to be used again.
    do
    {
        expectedIndex--;
        std::cout << "expected Index: " << +expectedIndex << std::endl;

        assert(indexToChannels.contains(expectedIndex));
        channelID chosenChannel = currentChannel.chooseNeighbour(indexToChannels.find(expectedIndex)->second, channelInformation);
        std::cout << "chosenChannel:" << channelIDToString(chosenChannel) << std::endl;

        useChannel(chosenChannel, channelInformation, connectionToSink, usedChannels);

        currentChannel = chosenChannel;
    } while (expectedIndex > 0);

    for (channelID channel : usedChannels)
        registerIndex(channelToIndex, indexToChannels, channel, constants::indexZero);

    return connectionToSink;
}

/* @return false if a net could not be routed with the given channelwidth, true if all nets were routed successfully
 */
bool routeNets(unsigned char const &arraySize, unsigned char const &channelwidth, std::map<std::string, std::shared_ptr<net>> &netsByNameOfTheSourceBlock,
               std::map<std::string, std::shared_ptr<block>> &blocks)
{
    std::map<channelID, channelInfo> channelInformation = generateChannelInformation(arraySize);

    unsigned short netIndex{};
    for (auto &netEntry : netsByNameOfTheSourceBlock)
    {
        std::string sourceBlockName = netEntry.first;
        std::shared_ptr<net> p_net = netEntry.second;

        // TODO for ex5p, before writing the routing file, "Routing net with sourceBlockName '[101]' and sink-blocks: '[3856]'" was printed 111 times!
        std::cout << "Routing net with sourceBlockName '" << sourceBlockName << "' and sink-blocks: '" << p_net->listConnectedBlocks() << "'" << std::endl;
        p_net->setIndex(netIndex);

        channelID sourceChannel = p_net->getSourceChannel();
        assert(channelInformation.contains(sourceChannel));
        if (channelInformation.find(sourceChannel)->second.isFull(channelwidth))
            return false;

        // stores the channels, that contain pins of this net and for each pin the associated blockname
        std::map<channelID, std::set<std::string>> relevantChannels = generateRelevantChannels(p_net, blocks);

        std::map<channelID, unsigned char> channelToIndex{};
        // can contain multiple entries for a channel - only the lowest is of relevance
        std::map<unsigned char, std::set<channelID>> indexToChannels{};
        unsigned short numberOfPinsReached{};

        /*         if (blocksConnectedToClock.contains(sourceBlockName))
                    blocksConnectedToClock.find(sourceBlockName)->second.first = netIndex; */

        registerIndex(channelToIndex, indexToChannels, sourceChannel, constants::indexZero);

        if (relevantChannels.contains(sourceChannel))
        {
            std::set<channelID> usedChannels{};
            std::stack<std::pair<channelID, unsigned char>> connectionToSink{};

            useChannel(sourceChannel, channelInformation, connectionToSink, usedChannels);

            for (std::string associatedBlockName : relevantChannels.find(sourceChannel)->second)
            {
                p_net->setConnection(associatedBlockName, connectionToSink);

                assert(blocks.contains(associatedBlockName));
                std::shared_ptr<block> p_block = blocks.find(associatedBlockName)->second;
                p_block->setChannelTaken(sourceChannel);
                removeOtherChannelEntries(relevantChannels, associatedBlockName, p_block->getOpenChannels());
            }
            numberOfPinsReached++;
            std::cout << "pin at source channel!" << std::endl;
        }

        while (numberOfPinsReached < p_net->getPinCount())
        {
            std::set<std::string> reachedBlocks;
            unsigned char indexOfSink{};
            channelID sink = findSink(channelToIndex, indexToChannels, arraySize, indexOfSink, channelwidth, channelInformation, relevantChannels, numberOfPinsReached, reachedBlocks, blocks);
            if (sink.getType() == 0)
                return false;

            std::stack<std::pair<channelID, unsigned char>> connectionToSink = retrace(arraySize, channelToIndex, indexToChannels, sink, indexOfSink, channelInformation);

            assert(reachedBlocks.size() != 0);
            for (std::string blockName : reachedBlocks)
            {
                std::cout << "reached block '" << blockName << "'" << std::endl;
                // 1-2 blocks
                p_net->setConnection(blockName, connectionToSink);
            }
        }
        std::cout << "------------------------------ net (sourceBlockName '" << sourceBlockName << "' - size: '" << p_net->getPinCount() << "') routed!" << std::endl;
        printConnections(p_net);
        assert(p_net->allPinsConnected());
        netIndex++;
    }

    return true;
}