#include <cassert>
#include "routing.hpp"
#include "block.hpp"
#include "logging.hpp"
#include "constants.hpp"
#include "net.hpp"

std::map<channelID, std::set<std::string>> generateRelevantChannels(std::shared_ptr<net> p_net, std::map<std::string, std::shared_ptr<block>> const &blocks)
{
    std::map<channelID, std::set<std::string>> relevantChannels{};
    for (std::string sinkBlockName : p_net->getSinkBlockNames())
    {
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

channelID findSink(std::map<channelID, unsigned char> &channelToIndex, std::map<unsigned char, std::set<channelID>> &indexToChannels, const unsigned char &arraySize, unsigned char &indexOfSink,
                   const unsigned char &channelwidth, auto &channelInformation, auto &relevantChannels, unsigned short &numberOfPinsReached, std::set<std::string> &reachedBlocks, const auto &blocks)
{
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

void useChannel(const channelID &channel, std::shared_ptr<net> p_net, auto &channelInformation, std::vector<std::pair<channelID, unsigned char>> &connectionToSink,
                std::set<channelID> &usedChannels, unsigned char &currentTrack)
{
    assert(channelInformation.contains(channel));

    if (p_net->usedChannel(channel))
        currentTrack = p_net->chooseUsedTrack(channel, currentTrack);
    else
    {
        currentTrack = channelInformation.find(channel)->second.useChannel(currentTrack);
        p_net->setUsedTrack(channel, currentTrack);
    }

    connectionToSink.emplace_back(channel, currentTrack);
    usedChannels.insert(channel);
}

std::vector<std::pair<channelID, unsigned char>> retrace(unsigned char arraySize, auto &channelToIndex, auto &indexToChannels, std::shared_ptr<net> p_net, const channelID &sink,
                                                         unsigned char &indexOfSink, auto &channelInformation)
{
    assert(sink.isInitialized());

    std::set<channelID> usedChannels{};
    std::vector<std::pair<channelID, unsigned char>> connectionToSink{};
    unsigned char currentTrack = std::numeric_limits<unsigned char>::max();

    useChannel(sink, p_net, channelInformation, connectionToSink, usedChannels, currentTrack);

    //printLogMessage("-------------\nsink: " + channelIDToString(sink));

    // printIndices(channelToIndex);

    channelID currentChannel = sink;
    unsigned char expectedIndex = indexOfSink;

    do
    {
        expectedIndex--;
        //printLogMessage("expected Index: " + std::to_string(+expectedIndex));

        assert(indexToChannels.contains(expectedIndex));
        channelID chosenChannel = currentChannel.chooseNeighbour(indexToChannels.find(expectedIndex)->second, currentTrack, channelInformation);
        //printLogMessage("chosenChannel:" + channelIDToString(chosenChannel));

        useChannel(chosenChannel, p_net, channelInformation, connectionToSink, usedChannels, currentTrack);

        currentChannel = chosenChannel;
    } while (expectedIndex > 0);

    for (channelID channel : usedChannels)
        registerIndex(channelToIndex, indexToChannels, channel, constants::indexZero);

    return connectionToSink;
}

/* @return false if a net could not be routed with the given channelwidth, true if all nets were routed successfully
 */
bool routeNets(unsigned char const &arraySize, unsigned char const &channelwidth, std::vector<std::shared_ptr<net>> const &sortedNets, std::map<std::string, std::shared_ptr<block>> const &blocks)
{
    std::map<channelID, channelInfo> channelInformation = generateChannelInformation(arraySize);

    for (unsigned short index = 0; index < sortedNets.size(); index++)
    {
        std::shared_ptr<net> p_net = sortedNets[index];

        //printLogMessage("Routing net with sourceBlockName '" + p_net->getSourceBlockName() + "' and " + std::to_string(p_net->getSinkBlockCount()) + " sink-blocks: '" + listConnectedBlocks(p_net) + "'");

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

        registerIndex(channelToIndex, indexToChannels, sourceChannel, constants::indexZero);

        if (relevantChannels.contains(sourceChannel))
        {
            std::set<channelID> usedChannels{};
            std::vector<std::pair<channelID, unsigned char>> connectionToSink{};
            unsigned char currentTrack = std::numeric_limits<unsigned char>::max();

            useChannel(sourceChannel, p_net, channelInformation, connectionToSink, usedChannels, currentTrack);

            // can be an adjacent block or the sourceBlock itself, if the net uses its output as input
            std::set<std::string> associatedBlockNames = relevantChannels.find(sourceChannel)->second;

            for (std::string associatedBlockName : associatedBlockNames)
            {
                p_net->setConnection(associatedBlockName, connectionToSink);

                assert(blocks.contains(associatedBlockName));
                std::shared_ptr<block> p_block = blocks.find(associatedBlockName)->second;
                p_block->setChannelTaken(sourceChannel);
                removeOtherChannelEntries(relevantChannels, associatedBlockName, p_block->getOpenChannels());

                //printLogMessage("pin of block '" + associatedBlockName + "' at source channel!\n");
                numberOfPinsReached++;
            }
        }

        while (numberOfPinsReached < p_net->getSinkBlockCount())
        {
            std::set<std::string> reachedBlocks{};
            unsigned char indexOfSink{};
            channelID sink = findSink(channelToIndex, indexToChannels, arraySize, indexOfSink, channelwidth, channelInformation, relevantChannels, numberOfPinsReached, reachedBlocks, blocks);
            if (!sink.isInitialized())
                return false;

            std::vector<std::pair<channelID, unsigned char>> connectionToSink = retrace(arraySize, channelToIndex, indexToChannels, p_net, sink, indexOfSink, channelInformation);

            assert(reachedBlocks.size() != 0);
            for (std::string blockName : reachedBlocks)
            {
                //printLogMessage("reached block '" + blockName + "'\n");
                // 1-2 blocks
                p_net->setConnection(blockName, connectionToSink);
            }
        }
        //printLogMessage("------------------------------ net routed!\n");

        assert(p_net->allPinsConnected());
    }

    return true;
}