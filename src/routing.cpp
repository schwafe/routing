#include "routing.hpp"
#include "block.hpp"

std::stack<channelID> retrace(unsigned char const &arraySize, auto &indices, channelID sink, unsigned char &indexOfSink, auto &channelInformation)
{
    std::stack<channelID> connectionToSink;
    connectionToSink.push(sink);
    channelInformation.find(sink)->second.useChannel();

    channelID currentChannel = sink;
    unsigned char expectedIndex = indexOfSink;
    do
    {
        expectedIndex--;
        channelID chosenChannel = currentChannel.chooseNeighbour(arraySize, indices, expectedIndex, channelInformation);

        channelInformation.find(chosenChannel)->second.useChannel();

        currentChannel = chosenChannel;
        connectionToSink.push(chosenChannel);
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
            for (std::string name : relevantChannels.find(channel)->second)
            {
                numberOfPinsReached++;
                reachedBlocks.insert(name);
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
                              auto &relevantChannels, unsigned short &numberOfPinsReached, channelID &sink, std::set<std::string> &reachedBlocks)
{
    unsigned char currentIndex = 0;

    std::set<channelID> currentWave(indexZeroChannels);
    std::set<channelID> newWave;
    do
    {
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
    } while (!currentWave.empty());

    // no pin found, net is not routable
    return -1;
}

bool routeNets(unsigned char const &arraySize, unsigned char const &maxTracks, std::map<std::string,std::shared_ptr<net>> &nets, std::map<std::string, block> &blocks)
{
    std::map<channelID, channelInfo> channelInformation = generateChannelInformation(arraySize);

    for (auto &entry : nets)
    {
        std::map<channelID, unsigned char> indices;
        unsigned short numberOfPinsReached;
        std::map<channelID, std::set<std::string>> relevantChannels;

        for (auto &entry : entry.second->getConnectedPinsAndTheirRouting())
        {
            std::string name = entry.first;

            for (channelID channel : blocks.find(name)->second.getOpenChannels())
            {
                if (relevantChannels.contains(channel))
                {
                    relevantChannels.find(channel)->second.insert(name);
                }
                else
                {
                    relevantChannels.emplace(channel, std::set<std::string>{name});
                }
            }
        }

        indices.emplace(entry.second->getSource(), 0);
        std::set<channelID> indexZeroChannels{entry.second->getSource()};

        while (numberOfPinsReached < entry.second->getPinCount())
        {
            channelID sink;
            std::set<std::string> reachedBlocks;
            unsigned char indexOfSink = wavePropagation(arraySize, indices, indexZeroChannels, maxTracks, channelInformation, relevantChannels,
                                                        numberOfPinsReached, sink, reachedBlocks);
            std::stack<channelID> connectionToSink = retrace(arraySize, indices, sink, indexOfSink, channelInformation);

            indexZeroChannels.insert(sink);
            for (std::string block : reachedBlocks)
            {
                // 1-2 blocks
                entry.second->setConnection(block, connectionToSink);
            }
        }
    }
    // TODO handle failure and report it
    return true;
}