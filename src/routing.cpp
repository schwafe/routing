#include <iostream>
#include "routing.hpp"
#include "block.hpp"

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

        currentChannel = chosenChannel;
        connectionToSink.emplace(chosenChannel, track);
    } while (expectedIndex > 0);

    return connectionToSink;
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
            std::cout << "relevant" << std::endl;
            for (std::string blockName : relevantChannels.find(channel)->second)
            {
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

bool routeNets(unsigned char const &arraySize, unsigned char const &maxTracks, std::map<std::string, std::shared_ptr<net>> &netsByNameOfTheSourceBlock,
               std::map<std::string, std::shared_ptr<block>> &blocks, std::map<std::string, std::pair<unsigned short, std::shared_ptr<block>>> &blocksConnectedToClock, std::string &clockName)
{
    std::map<channelID, channelInfo> channelInformation = generateChannelInformation(arraySize);

    unsigned short netIndex = 0;
    for (auto &netEntry : netsByNameOfTheSourceBlock)
    {
        netEntry.second->setIndex(netIndex);

        std::map<channelID, unsigned char> indices;
        unsigned short numberOfPinsReached = 0;
        std::map<channelID, std::set<std::string>> relevantChannels;

        if (blocksConnectedToClock.contains(netEntry.first))
            blocksConnectedToClock.find(netEntry.first)->second.first = netIndex;

        std::cout << "getConnectedPinBlockNamesAndTheirRouting size " << netEntry.second->getConnectedPinBlockNamesAndTheirRouting().size() << std::endl;
        for (auto &connectedPinEntry : netEntry.second->getConnectedPinBlockNamesAndTheirRouting())
        {
            std::string blockName = connectedPinEntry.first;

            std::cout << "sourceBlock " << netEntry.first << std::endl
                      << "connected pin blockName " << blockName << std::endl
                      << "openChannels size " << blocks.find(blockName)->second->getOpenChannels().size() << std::endl;
            for (channelID channel : blocks.find(blockName)->second->getOpenChannels())
            {
                if (relevantChannels.contains(channel))
                {
                    relevantChannels.find(channel)->second.insert(blockName);
                }
                else
                {
                    relevantChannels.emplace(channel, std::set<std::string>{blockName});
                }
            }
        }
        std::cout << "relevant channels size " << relevantChannels.size() << std::endl;

        channelID sourceChannel = netEntry.second->getSourceChannel();
        if (channelInformation.find(sourceChannel)->second.isFull(maxTracks))
            return false;

        std::cout << "x " << +sourceChannel.getXCoordinate() << " y " << +sourceChannel.getYCoordinate() << " type " << sourceChannel.getType() << std::endl;
        if (relevantChannels.contains(sourceChannel))
        {
            std::stack<std::pair<channelID, unsigned char>> connectionToSink;
            unsigned char track = channelInformation.find(sourceChannel)->second.useChannel();
            connectionToSink.emplace(sourceChannel, track);
            std::string reachedBlock = *relevantChannels.find(sourceChannel)->second.cbegin();
            netEntry.second->setConnection(reachedBlock, connectionToSink);
            numberOfPinsReached++;
            std::cout << "pin at source channel!" << std::endl;
        }

        indices.emplace(sourceChannel, 0);
        std::set<channelID> indexZeroChannels{sourceChannel};

        std::cout << "net pin count: " << netEntry.second->getPinCount() << std::endl;
        while (numberOfPinsReached < netEntry.second->getPinCount())
        {
            channelID sink;
            std::set<std::string> reachedBlocks;
            unsigned char indexOfSink = wavePropagation(arraySize, indices, indexZeroChannels, maxTracks, channelInformation, relevantChannels,
                                                        numberOfPinsReached, sink, reachedBlocks);
            std::stack<std::pair<channelID, unsigned char>> connectionToSink = retrace(arraySize, indices, sink, indexOfSink, channelInformation);

            indexZeroChannels.insert(sink);
            for (std::string blockName : reachedBlocks)
            {
                // 1-2 blocks
                netEntry.second->setConnection(blockName, connectionToSink);
            }

            std::cout << "pin reached!" << std::endl;
        }
        std::cout << "net routed!" << std::endl;
        netIndex++;
    }
    // TODO handle failure and report it
    return true;
}