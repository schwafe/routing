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

void removeOtherChannelEntries(std::map<channelID, std::set<std::string>> relevantChannels,std::shared_ptr<block> block) {
    //TODO
    for(channelID channel : block->getOpenChannels()) {
        
    }
}

/* @return true if pin was found, false otherwise
 */
bool addChannelIfValid(channelID channel, auto &indices, unsigned char index, unsigned char const &maxTracks, auto &channelInformation, auto &newWave, auto &relevantChannels,
                       unsigned short &numberOfPinsReached, channelID &sink, std::set<std::string> &reachedBlocks, auto &blocks)
{
    assert(channelInformation.contains(channel));
    if (!indices.contains(channel) && !channelInformation.find(channel)->second.isFull(maxTracks))
    {
        indices.emplace(channel, index);

        if (relevantChannels.contains(channel))
        {
            for (std::string blockName : relevantChannels.find(channel)->second)
            {
                numberOfPinsReached++;
                reachedBlocks.insert(blockName);
                assert(blocks.contains(blockName));
                std::shared_ptr<block> p_block =  blocks.find(blockName)->second;
                p_block->setChannelTaken(channel);
                removeOtherChannelEntries(relevantChannels, p_block);
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

unsigned char wavePropagation(unsigned char const &arraySize, auto &indices, std::set<channelID> &indexZeroChannels, unsigned char const &maxTracks, auto &channelInformation,
                              auto &relevantChannels, unsigned short &numberOfPinsReached, channelID &sink, std::set<std::string> &reachedBlocks, auto &blocks)
{
    unsigned char currentIndex = 1;

    std::set<channelID> currentWave(indexZeroChannels);
    std::set<channelID> newWave;
    do
    {
        for (channelID channel : currentWave)
        {
            for (channelID neighbour : channel.getNeighbours(arraySize))
            {
                bool pinFound = addChannelIfValid(neighbour, indices, currentIndex, maxTracks, channelInformation, newWave, relevantChannels, numberOfPinsReached, sink, reachedBlocks, blocks);
                if (pinFound)
                    return currentIndex;
            }
        }
        currentIndex++;

        currentWave.clear();
        currentWave = newWave;
        newWave.clear();
        // TODO improve abort condition
    } while (!currentWave.empty());

    // no pin found, net is not routable
    return -1;
}
void useChannel(channelID &channel, auto &channelInformation, std::stack<std::pair<channelID, unsigned char>> &connectionToSink, std::set<channelID> &usedChannels)
{
    assert(channelInformation.contains(channel));
    unsigned char track = channelInformation.find(channel)->second.useChannel();
    connectionToSink.emplace(channel, track);
    usedChannels.insert(channel);
}

std::stack<std::pair<channelID, unsigned char>> retrace(unsigned char const &arraySize, auto &indices, channelID sink, unsigned char &indexOfSink, auto &channelInformation)
{
    std::set<channelID> usedChannels{};
    std::stack<std::pair<channelID, unsigned char>> connectionToSink{};
    useChannel(sink, channelInformation, connectionToSink, usedChannels);

    std::cout << std::endl
              << std::endl;
    printIndices(indices);

    std::cout << "sink: " << channelIDToString(sink) << std::endl;

    channelID currentChannel = sink;
    unsigned char expectedIndex = indexOfSink;
    // TODO most likely using channels at index 0 more than one time. Connection to sink (and the retrace step) need to stop before a channel is used twice. For the first connection, this is no problem, but all the following ones need to stop at index 1.
    // not quite correct? connectionToSink does need the index 0 element, but if the track is the same, the channel does not need to be used again.
    do
    {
        expectedIndex--;
        std::cout << "expected Index: " << +expectedIndex << std::endl;
        channelID chosenChannel = currentChannel.chooseNeighbour(arraySize, indices, expectedIndex, channelInformation);
        std::cout << "chosenChannel:" << channelIDToString(chosenChannel) << std::endl;

        useChannel(chosenChannel, channelInformation, connectionToSink, usedChannels);

        currentChannel = chosenChannel;
    } while (expectedIndex > 0);

    for (channelID channel : usedChannels)
        indices.insert_or_assign(channel, constants::indexZero);

    return connectionToSink;
}

bool routeNets(unsigned char const &arraySize, unsigned char const &maxTracks, std::map<std::string, std::shared_ptr<net>> &netsByNameOfTheSourceBlock,
               std::map<std::string, std::shared_ptr<block>> &blocks)
{
    std::map<channelID, channelInfo> channelInformation = generateChannelInformation(arraySize);

    unsigned short netIndex{};
    for (auto &netEntry : netsByNameOfTheSourceBlock)
    {
        std::string sourceBlockName = netEntry.first;
        std::shared_ptr<net> p_net = netEntry.second;

        std::cout << "Routing net with sourceBlockName '" << sourceBlockName << "' and sink-blocks: '" << p_net->listConnectedBlocks() << "'" << std::endl;
        p_net->setIndex(netIndex);

        channelID sourceChannel = p_net->getSourceChannel();
        assert(channelInformation.contains(sourceChannel));
        if (channelInformation.find(sourceChannel)->second.isFull(maxTracks))
            return false;

        std::map<channelID, unsigned char> indices;
        unsigned short numberOfPinsReached{};
        // stores the channels, that contain pins of this net and for each pin the associated blockname
        std::map<channelID, std::set<std::string>> relevantChannels = generateRelevantChannels(p_net, blocks);

        /*         if (blocksConnectedToClock.contains(sourceBlockName))
                    blocksConnectedToClock.find(sourceBlockName)->second.first = netIndex; */

        indices.emplace(sourceChannel, constants::indexZero);

        if (relevantChannels.contains(sourceChannel))
        {
            std::set<channelID> usedChannels{};
            std::stack<std::pair<channelID, unsigned char>> connectionToSink{};
            useChannel(sourceChannel, channelInformation, connectionToSink, usedChannels);
            std::set<std::string> associatedBlocks = relevantChannels.find(sourceChannel)->second;
            for (std::string associatedBlockName : associatedBlocks) {
                //TODO check tomorrow
                p_net->setConnection(associatedBlockName, connectionToSink);

                assert(blocks.contains(associatedBlockName));
                std::shared_ptr<block> p_block = blocks.find(associatedBlockName)->second;
                p_block->setChannelTaken(sourceChannel);
                removeOtherChannelEntries(relevantChannels, p_block);
            }
            numberOfPinsReached++;
            std::cout << "pin at source channel!" << std::endl;
        }

        std::set<channelID> indexZeroChannels{sourceChannel};

        while (numberOfPinsReached < p_net->getPinCount())
        {
            channelID sink;
            std::set<std::string> reachedBlocks;
            unsigned char indexOfSink = wavePropagation(arraySize, indices, indexZeroChannels, maxTracks, channelInformation, relevantChannels,
                                                        numberOfPinsReached, sink, reachedBlocks, blocks);
            std::stack<std::pair<channelID, unsigned char>> connectionToSink = retrace(arraySize, indices, sink, indexOfSink, channelInformation);

            indexZeroChannels.insert(sink);
            for (std::string blockName : reachedBlocks)
            {
                std::cout << "reached block '" << blockName << "'" << std::endl;
                // 1-2 blocks
                p_net->setConnection(blockName, connectionToSink);
            }
        }
        std::cout << "------------------------------ net (sourceBlockName '" << sourceBlockName << "' - size: '" << p_net->getPinCount() << "') routed!" << std::endl;
        printConnections(*p_net);
        assert(p_net->allPinsConnected());
        netIndex++;
    }
    // TODO handle failure and report it
    return true;
}