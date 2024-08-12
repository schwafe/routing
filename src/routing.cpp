#include <iostream>
#include <cassert>
#include "routing.hpp"
#include "block.hpp"

std::map<channelID, std::set<std::string>> generateRelevantChannels(std::shared_ptr<net> p_net, std::map<std::string, std::shared_ptr<block>> &blocks)
{
    std::map<channelID, std::set<std::string>> relevantChannels;
    for (auto &connectedPinEntry : p_net->getConnectedPinBlockNamesAndTheirRouting())
    {
        std::string sinkBlockName = connectedPinEntry.first;

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

/* @return true if pin was found, false otherwise
 */
bool addChannelIfValid(channelID channel, auto &indices, unsigned char index, unsigned char const &maxTracks, auto &channelInformation, auto &newWave, auto &relevantChannels,
                       unsigned short &numberOfPinsReached, channelID &sink, std::set<std::string> &reachedBlocks)
{
    if (!indices.contains(channel) && !channelInformation.find(channel)->second.isFull(maxTracks))
    {
        indices.emplace(channel, index);

        if (relevantChannels.contains(channel))
        {
            std::cout << "relevant";
            for (std::string blockName : relevantChannels.find(channel)->second)
            {
                std::cout << " block " << blockName << std::endl;
                numberOfPinsReached++;
                reachedBlocks.insert(blockName);
            }
            relevantChannels.erase(channel);
            sink = channel;
            return true;
        }
        else
        {
            std::cout << "newWave" << std::endl;
            newWave.insert(channel);
        }
    }
    else
    {
        std::cout << "invalid" << std::endl;
    }
    return false;
}

unsigned char wavePropagation(unsigned char const &arraySize, auto &indices, std::set<channelID> &indexZeroChannels, unsigned char const &maxTracks, auto &channelInformation,
                              auto &relevantChannels, unsigned short &numberOfPinsReached, channelID &sink, std::set<std::string> &reachedBlocks)
{
    unsigned char currentIndex = 0;

    std::set<channelID> currentWave(indexZeroChannels);
    std::set<channelID> newWave;
    do
    {
        std::cout << "current index " << +currentIndex << std::endl;
        for (channelID channel : currentWave)
        {
            for (channelID neighbour : channel.getNeighbours(arraySize))
            {
                bool pinFound = addChannelIfValid(neighbour, indices, currentIndex, maxTracks, channelInformation, newWave, relevantChannels, numberOfPinsReached, sink, reachedBlocks);
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

std::stack<std::pair<channelID, unsigned char>> retrace(unsigned char const &arraySize, auto &indices, channelID sink, unsigned char &indexOfSink, auto &channelInformation)
{
    std::stack<std::pair<channelID, unsigned char>> connectionToSink;
    unsigned char track = channelInformation.find(sink)->second.useChannel();
    connectionToSink.emplace(sink, track);

    channelID currentChannel = sink;
    unsigned char expectedIndex = indexOfSink;
    // TODO most likely using channels at index 0 more than one time. Connection to sink (and the retrace step) need to stop before a channel is used twice. For the first connection, this is no problem, but all the following ones need to stop at index 1.
    // not quite correct? connectionToSink does need the index 0 element, but if the track is the same, the channel does not need to be used again.
    do
    {
        expectedIndex--;
        channelID chosenChannel = currentChannel.chooseNeighbour(arraySize, indices, expectedIndex, channelInformation);

        track = channelInformation.find(chosenChannel)->second.useChannel();
        connectionToSink.emplace(chosenChannel, track);

        currentChannel = chosenChannel;
    } while (expectedIndex > 0);

    return connectionToSink;
}

bool routeNets(unsigned char const &arraySize, unsigned char const &maxTracks, std::map<std::string, std::shared_ptr<net>> &netsByNameOfTheSourceBlock,
               std::map<std::string, std::shared_ptr<block>> &blocks)
{
    std::map<channelID, channelInfo> channelInformation = generateChannelInformation(arraySize);

    unsigned short netIndex = 0;
    for (auto &netEntry : netsByNameOfTheSourceBlock)
    {
        std::string sourceBlockName = netEntry.first;
        std::shared_ptr<net> p_net = netEntry.second;

        std::cout << "Routing net with sourceBlockName '" << sourceBlockName << "' and sink-blocks: '" << p_net->listConnectedBlocks() << "'" << std::endl;
        p_net->setIndex(netIndex);

        channelID sourceChannel = p_net->getSourceChannel();
        if (channelInformation.find(sourceChannel)->second.isFull(maxTracks))
            return false;

        std::map<channelID, unsigned char> indices;
        unsigned short numberOfPinsReached = 0;
        // stores the channels, that contain pins of this net and for each pin the associated blockname
        std::map<channelID, std::set<std::string>> relevantChannels = generateRelevantChannels(p_net, blocks);

        /*         if (blocksConnectedToClock.contains(sourceBlockName))
                    blocksConnectedToClock.find(sourceBlockName)->second.first = netIndex; */

        if (relevantChannels.contains(sourceChannel))
        {
            std::stack<std::pair<channelID, unsigned char>> connectionToSink;
            unsigned char track = useChannel(sourceChannel, channelInformation);
            connectionToSink.emplace(sourceChannel, track);
            std::set<std::string> associatedBlocks = relevantChannels.find(sourceChannel)->second;
            assert(associatedBlocks.size() == 1);
            for (std::string block : associatedBlocks)
                p_net->setConnection(block, connectionToSink);
            numberOfPinsReached++;
            std::cout << "pin at source channel!" << std::endl;
        }

        indices.emplace(sourceChannel, 0);
        std::set<channelID> indexZeroChannels{sourceChannel};

        while (numberOfPinsReached < p_net->getPinCount())
        {
            channelID sink;
            std::set<std::string> reachedBlocks;
            unsigned char indexOfSink = wavePropagation(arraySize, indices, indexZeroChannels, maxTracks, channelInformation, relevantChannels,
                                                        numberOfPinsReached, sink, reachedBlocks);
            std::stack<std::pair<channelID, unsigned char>> connectionToSink = retrace(arraySize, indices, sink, indexOfSink, channelInformation);
            assert(!connectionToSink.empty());

            indexZeroChannels.insert(sink);
            for (std::string blockName : reachedBlocks)
            {
                // 1-2 blocks
                p_net->setConnection(blockName, connectionToSink);
            }

            std::cout << "pin reached!" << std::endl;
        }
        std::cout << "------------------------------ net (sourceBlockName '" << sourceBlockName << "' - size: '"<< p_net->getPinCount()<<"') routed!" << std::endl;
        assert(p_net->allPinsConnected());
        netIndex++;
    }
    // TODO handle failure and report it
    return true;
}