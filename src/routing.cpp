#include <cassert>
#include "routing.hpp"
#include "block.hpp"
#include "logging.hpp"
#include "constants.hpp"
#include "net.hpp"

std::map<channelID, std::set<std::string>> generateRelevantChannels(std::set<channelID> &doublyRelevantChannels, std::shared_ptr<net> const &p_net, std::map<std::string, std::shared_ptr<block>> const &blocks)
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
                assert(relevantChannels.find(channel)->second.size() == 2);
                assert(!doublyRelevantChannels.contains(channel));
                doublyRelevantChannels.insert(channel);
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

void registerIndex(std::map<channelID, unsigned char> &channelToIndex, std::map<unsigned char, std::set<channelID>> &indexToChannels, channelID channel, unsigned char index)
{
    assert(!channelToIndex.contains(channel) || channelToIndex.find(channel)->second > index || (channelToIndex.find(channel)->second == constants::indexZero && index == constants::indexZero));
    channelToIndex.insert_or_assign(channel, index);
    if (!indexToChannels.contains(index))
        indexToChannels.emplace(index, std::set<channelID>{});
    indexToChannels.find(index)->second.emplace(channel);
}

void updateBlocksAndRelevantChannels(channelID const &channel, std::string const &blockName, std::map<channelID, std::set<std::string>> &relevantChannels, std::set<channelID> &doublyRelevantChannels, auto const &blocks)
{
    assert(blocks.contains(blockName));
    std::shared_ptr<block> p_block = blocks.find(blockName)->second;
    p_block->setChannelTaken(channel);

    for (channelID channel : p_block->getOpenChannels())
    {
        assert(relevantChannels.contains(channel));
        std::set<std::string> &associatedBlocks = relevantChannels.find(channel)->second;
        associatedBlocks.erase(blockName);
        if (!associatedBlocks.empty())
        {
            assert(doublyRelevantChannels.contains(channel));
            doublyRelevantChannels.erase(channel);
        }
        else
            relevantChannels.erase(channel);
    }
}

channelID findSink(std::map<channelID, unsigned char> &channelToIndex, std::map<unsigned char, std::set<channelID>> &indexToChannels, unsigned char arraySize, unsigned char &indexOfSink,
                   unsigned char channelwidth, auto const &channelInformation, auto &relevantChannels, std::set<channelID> &doublyRelevantChannels,
                   unsigned short &numberOfPinsReached, std::set<std::string> &reachedBlocks, auto const &blocks)
{
    std::set<channelID> processedChannels{};
    unsigned char currentIndex = constants::indexZero;
    assert(indexToChannels.contains(constants::indexZero));

    bool relevantChannelFound{};
    std::pair<channelID, unsigned char> firstRelevantChannelFoundWithIndex{channelID{}, std::numeric_limits<unsigned char>::max()};
    do
    {
        unsigned char expectedIndex = currentIndex + 1;

        for (channelID channel : indexToChannels.find(currentIndex)->second)
        {
            if (processedChannels.contains(channel))
                continue;

            for (channelID neighbour : channel.getNeighbours(arraySize))
            {
                assert(channelInformation.contains(neighbour));

                if (auto it = channelToIndex.find(neighbour); it != channelToIndex.end())
                {
                    assert(it->second >= currentIndex - 1);
                    if (it->second > expectedIndex)
                    {
                        it->second = expectedIndex;

                        auto result = indexToChannels.find(expectedIndex);
                        if (result == indexToChannels.end())
                            result = indexToChannels.emplace(expectedIndex, std::set<channelID>{}).first;
                        result->second.emplace(neighbour);
                    }

                    if (!relevantChannelFound && relevantChannels.contains(neighbour))
                    {
                        assert(relevantChannels.find(neighbour)->second.size() == 1);
                        firstRelevantChannelFoundWithIndex = std::make_pair(neighbour, expectedIndex);
                        relevantChannelFound = true;
                    }
                }
                else if (!channelInformation.find(neighbour)->second.isFull(channelwidth))
                {
                    registerIndex(channelToIndex, indexToChannels, neighbour, expectedIndex);

                    if (auto it = doublyRelevantChannels.find(neighbour); it != doublyRelevantChannels.end())
                    {
                        assert(relevantChannels.find(neighbour)->second.size() == 2);
                        for (std::string associatedBlockName : relevantChannels.find(neighbour)->second)
                        {
                            reachedBlocks.insert(associatedBlockName);
                            updateBlocksAndRelevantChannels(neighbour, associatedBlockName, relevantChannels, doublyRelevantChannels, blocks);
                        }

                        relevantChannels.erase(neighbour);
                        doublyRelevantChannels.erase(it);

                        numberOfPinsReached += 2;
                        indexOfSink = expectedIndex;
                        return neighbour;
                    }
                    else if (relevantChannels.contains(neighbour))
                    {
                        assert(relevantChannels.find(neighbour)->second.size() == 1);
                        firstRelevantChannelFoundWithIndex = std::make_pair(neighbour, expectedIndex);
                        relevantChannelFound = true;
                    }
                }
            }

            processedChannels.emplace(channel);
        }
    } while (indexToChannels.contains(++currentIndex) && (!relevantChannelFound || (!doublyRelevantChannels.empty() && firstRelevantChannelFoundWithIndex.second >= currentIndex - 2)));

    if (relevantChannelFound)
    {
        channelID relevantChannel = firstRelevantChannelFoundWithIndex.first;

        assert(relevantChannels.find(relevantChannel)->second.size() == 1);
        for (std::string associatedBlockName : relevantChannels.find(relevantChannel)->second)
        {
            reachedBlocks.insert(associatedBlockName);
            updateBlocksAndRelevantChannels(relevantChannel, associatedBlockName, relevantChannels, doublyRelevantChannels, blocks);
        }

        relevantChannels.erase(relevantChannel);

        numberOfPinsReached++;
        indexOfSink = firstRelevantChannelFoundWithIndex.second;
        return relevantChannel;
    }
    else
        return channelID{};
}

void useChannel(channelID channel, std::shared_ptr<net> const &p_net, std::map<channelID, channelInfo> &channelInformation, std::vector<std::pair<channelID, unsigned char>> &connectionToSink,
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

std::vector<std::pair<channelID, unsigned char>> retrace(auto &channelToIndex, auto &indexToChannels, std::shared_ptr<net> const &p_net, channelID sink, unsigned char indexOfSink,
                                                         std::map<channelID, channelInfo> &channelInformation)
{
    assert(sink.isInitialized());

    std::set<channelID> usedChannels{};
    std::vector<std::pair<channelID, unsigned char>> connectionToSink{};
    unsigned char currentTrack = std::numeric_limits<unsigned char>::max();

    useChannel(sink, p_net, channelInformation, connectionToSink, usedChannels, currentTrack);

    // printLogMessage("-------------\nsink: " + channelIDToString(sink));

    // printIndices(channelToIndex);

    channelID currentChannel = sink;
    unsigned char expectedIndex = indexOfSink;

    do
    {
        expectedIndex--;
        // printLogMessage("expected Index: " + std::to_string(+expectedIndex));

        assert(indexToChannels.contains(expectedIndex));
        channelID chosenChannel = currentChannel.chooseNeighbour(indexToChannels.find(expectedIndex)->second, currentTrack, channelInformation);
        // printLogMessage("chosenChannel:" + channelIDToString(chosenChannel));

        useChannel(chosenChannel, p_net, channelInformation, connectionToSink, usedChannels, currentTrack);

        currentChannel = chosenChannel;
    } while (expectedIndex > 0);

    for (channelID channel : usedChannels)
        registerIndex(channelToIndex, indexToChannels, channel, constants::indexZero);

    return connectionToSink;
}

/* @return false if a net could not be routed with the given channelwidth, true if all nets were routed successfully
 */
bool routeNets(unsigned char arraySize, unsigned char channelwidth, std::vector<std::shared_ptr<net>> const &sortedNets, std::map<std::string, std::shared_ptr<block>> const &blocks)
{
    std::map<channelID, channelInfo> channelInformation = generateChannelInformation(arraySize);

    for (unsigned short index = 0; index < sortedNets.size(); index++)
    {
        std::shared_ptr<net> p_net = sortedNets[index];

        // printLogMessage("Routing net with sourceBlockName '" + p_net->getSourceBlockName() + "' and " + std::to_string(p_net->getSinkBlockCount()) + " sink-blocks: '" + listConnectedBlocks(p_net) + "'");

        channelID sourceChannel = p_net->getSourceChannel();
        assert(channelInformation.contains(sourceChannel));
        if (channelInformation.find(sourceChannel)->second.isFull(channelwidth))
            return false;

        std::set<channelID> doublyRelevantChannels{};
        // stores the channels, that contain pins of this net and for each pin the associated blockname
        std::map<channelID, std::set<std::string>> relevantChannels = generateRelevantChannels(doublyRelevantChannels, p_net, blocks);

        std::map<channelID, unsigned char> channelToIndex{};
        // can contain multiple entries for a channel - only the lowest is of relevance
        std::map<unsigned char, std::set<channelID>> indexToChannels{};
        unsigned short numberOfPinsReached{};

        registerIndex(channelToIndex, indexToChannels, sourceChannel, constants::indexZero);

        auto it = relevantChannels.find(sourceChannel);
        if (it != relevantChannels.end())
        {
            std::set<channelID> usedChannels{};
            std::vector<std::pair<channelID, unsigned char>> connectionToSink{};
            unsigned char currentTrack = std::numeric_limits<unsigned char>::max();

            useChannel(sourceChannel, p_net, channelInformation, connectionToSink, usedChannels, currentTrack);

            // can be an adjacent block or the sourceBlock itself, if the net uses its output as input
            std::set<std::string> associatedBlockNames = it->second;

            for (std::string associatedBlockName : associatedBlockNames)
            {
                updateBlocksAndRelevantChannels(sourceChannel, associatedBlockName, relevantChannels, doublyRelevantChannels, blocks);

                numberOfPinsReached++;
                p_net->setConnection(associatedBlockName, connectionToSink);
            }

            relevantChannels.erase(it);
            doublyRelevantChannels.erase(sourceChannel);
        }

        while (numberOfPinsReached < p_net->getSinkBlockCount())
        {
            std::set<std::string> reachedBlocks{};
            unsigned char indexOfSink{};
            channelID sink = findSink(channelToIndex, indexToChannels, arraySize, indexOfSink, channelwidth, channelInformation, relevantChannels, doublyRelevantChannels,
                                      numberOfPinsReached, reachedBlocks, blocks);
            if (!sink.isInitialized())
                return false;

            std::vector<std::pair<channelID, unsigned char>> connectionToSink = retrace(channelToIndex, indexToChannels, p_net, sink, indexOfSink, channelInformation);

            assert(reachedBlocks.size() != 0);
            for (std::string blockName : reachedBlocks)
            {
                // printLogMessage("reached block '" + blockName + "'\n");
                //  1-2 blocks
                p_net->setConnection(blockName, connectionToSink);
            }
        }
        // printLogMessage("------------------------------ net routed!\n");

        assert(p_net->allPinsConnected());
    }

    return true;
}