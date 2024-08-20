#include <cassert>
#include "routing.hpp"
#include "block.hpp"
#include "logging.hpp"
#include "constants.hpp"
#include "net.hpp"

std::map<channelID, std::set<std::string>> findRelevantChannels(std::map<channelID, std::set<std::string>> &relevantChannels, std::set<channelID> &doublyRelevantChannels,
                                                                std::shared_ptr<net> const &p_net, std::map<std::string, std::shared_ptr<block>> const &blocks)
{
    for (std::string connectedBlockName : p_net->getNamesOfConnectedBlocks())
    {
        assert(blocks.contains(connectedBlockName));
        for (channelID channelWithPin : blocks.find(connectedBlockName)->second->getOpenChannels())
        {
            if (auto it = relevantChannels.find(channelWithPin); it != relevantChannels.end())
            {
                it->second.insert(connectedBlockName);
                assert(relevantChannels.find(channelWithPin)->second.size() == 2);
                assert(!doublyRelevantChannels.contains(channelWithPin));
                doublyRelevantChannels.insert(channelWithPin);
            }
            else
            {
                std::set<std::string> associatedBlocks{connectedBlockName};
                relevantChannels.insert(std::make_pair(channelWithPin, associatedBlocks));
            }
        }
    }
    return relevantChannels;
}

/* Adds or overwrites the entry in channelToIndex with the given index and adds the channel to the set of channels for the given index in indexToChannels.
Other entries for the same channel may exist in indexToChannels. */
void registerIndex(channelID channel, unsigned char index, std::map<channelID, unsigned char> &channelToIndex, std::map<unsigned char, std::set<channelID>> &indexToChannels)
{
    assert(!channelToIndex.contains(channel) || channelToIndex.find(channel)->second > index || (channelToIndex.find(channel)->second == constants::indexZero && index == constants::indexZero));
    channelToIndex.insert_or_assign(channel, index);
    if (!indexToChannels.contains(index))
        indexToChannels.emplace(index, std::set<channelID>{});
    indexToChannels.find(index)->second.emplace(channel);
}

/* TODO split up? */
void updateBlockAndRelevantChannels(channelID const &channel, std::shared_ptr<block> const &p_block, std::map<channelID, std::set<std::string>> &relevantChannels, std::set<channelID> &doublyRelevantChannels, auto const &blocks)
{
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

/* Finds a pin by searching neighbouring channels of the channels in indexToChannels at index zero. Updates indexToChannels and channelToIndex to find relevant channels faster in the next
iteration. RelevantChannels and doublyRelevantChannels indicate which channels contain pins and are updated before returning the chosen channel.

Starts with one or more channels, that are already part of the net.
Searches the neighbouring channels of the channels, that are have already been searched (or in the first round the channels, that are part of the net).
If a channel, that contains a pin, that needs to be connected to the net, ('relevant channel') is found, this is noted. If channels exist, that contain two pins, that need to be connected,
('doubly relevant channel') the search continues for two iterations or until such a channel is found. If no doubly relevant channel is found, the first relevant channel, that was found, is
returned. If no doubly relevant channel exists, the search stops as soon as any relevant channel is found.
    @return if the search is successful: the chosen channel, that contains or two pins, that need to be connected to the net - if not: a channel created with the default constructor
*/
channelID findPin(std::map<channelID, unsigned char> &channelToIndex, std::map<unsigned char, std::set<channelID>> &indexToChannels, unsigned char arraySize, unsigned char &indexOfSink,
                  unsigned char channelWidth, auto const &channelInformation, auto &relevantChannels, std::set<channelID> &doublyRelevantChannels,
                  unsigned short &numberOfPinsReached, std::set<std::string> &reachedBlocks, std::map<std::string, std::shared_ptr<block>> const &blocks)
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
                else if (isChannelFull(neighbour, channelInformation, channelWidth))
                {
                    registerIndex(channelToIndex, indexToChannels, neighbour, expectedIndex);

                    if (auto it = doublyRelevantChannels.find(neighbour); it != doublyRelevantChannels.end())
                    {
                        assert(relevantChannels.find(neighbour)->second.size() == 2);
                        for (std::string associatedBlockName : relevantChannels.find(neighbour)->second)
                        {
                            reachedBlocks.insert(associatedBlockName);
                            updateBlockAndRelevantChannels(neighbour, blocks.find(associatedBlockName)->second, relevantChannels, doublyRelevantChannels, blocks);
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
            updateBlockAndRelevantChannels(relevantChannel, blocks.find(associatedBlockName)->second, relevantChannels, doublyRelevantChannels, blocks);
        }

        relevantChannels.erase(relevantChannel);

        numberOfPinsReached++;
        indexOfSink = firstRelevantChannelFoundWithIndex.second;
        return relevantChannel;
    }
    else
        return channelID{};
}

void useChannel(channelID channel, std::shared_ptr<net> const &p_net, std::map<channelID, channelInfo> &channelInformation, std::vector<std::pair<channelID, unsigned char>> &connectionToBlock,
                std::set<channelID> &usedChannels, unsigned char &currentTrack)
{
    if (p_net->usedChannel(channel))
        currentTrack = p_net->chooseUsedTrack(channel, currentTrack);
    else
    {
        currentTrack = useChannel(channel, channelInformation, currentTrack);
        p_net->setUsedTrack(channel, currentTrack);
    }

    connectionToBlock.emplace_back(channel, currentTrack);
    usedChannels.insert(channel);
}

std::vector<std::pair<channelID, unsigned char>> retrace(auto &channelToIndex, auto &indexToChannels, std::shared_ptr<net> const &p_net, channelID sink, unsigned char indexOfSink,
                                                         std::map<channelID, channelInfo> &channelInformation)
{
    assert(sink.isInitialized());

    std::set<channelID> usedChannels{};
    std::vector<std::pair<channelID, unsigned char>> connectionToBlock{};
    unsigned char currentTrack = std::numeric_limits<unsigned char>::max();

    useChannel(sink, p_net, channelInformation, connectionToBlock, usedChannels, currentTrack);

    channelID currentChannel = sink;
    unsigned char expectedIndex = indexOfSink;

    do
    {
        expectedIndex--;

        assert(indexToChannels.contains(expectedIndex));
        channelID chosenChannel = chooseNeighbouringChannel(currentChannel, indexToChannels.find(expectedIndex)->second, currentTrack, channelInformation);

        useChannel(chosenChannel, p_net, channelInformation, connectionToBlock, usedChannels, currentTrack);

        currentChannel = chosenChannel;
    } while (expectedIndex > 0);

    for (channelID channel : usedChannels)
        registerIndex(channel, constants::indexZero, channelToIndex, indexToChannels);

    return connectionToBlock;
}

/* @return false if a net could not be routed with the given channelWidth, true if all nets were routed successfully
 */
bool routeNets(unsigned char arraySize, unsigned char channelWidth, std::vector<std::shared_ptr<net>> const &sortedNets, std::map<std::string, std::shared_ptr<block>> const &blocks)
{
    std::map<channelID, channelInfo> channelInformation = generateChannelInformation(arraySize);

    for (std::shared_ptr<net> const &p_net : sortedNets)
    {
        channelID sourceChannel = p_net->getSourceChannel();
        if (isChannelFull(sourceChannel, channelInformation, channelWidth))
        {
            printLogMessage("Routing failed due to the source channel of block '" + p_net->getSourceBlockName() + "' being full.");
            return false;
        }

        /* stores channels, that contain input pins of two blocks, that are to be connected to the net*/
        std::set<channelID> doublyRelevantChannels{};
        /* stores channels, that contain pins of blocks, that are to be connected to the net and for each channel the names of the blocks */
        std::map<channelID, std::set<std::string>> relevantChannels{};
        findRelevantChannels(relevantChannels, doublyRelevantChannels, p_net, blocks);

        std::map<channelID, unsigned char> channelToIndex{};
        // can contain multiple entries for a channel - only the lowest is of relevance
        std::map<unsigned char, std::set<channelID>> indexToChannels{};
        unsigned short numberOfPinsReached{};

        registerIndex(sourceChannel, constants::indexZero, channelToIndex, indexToChannels);

        auto it = relevantChannels.find(sourceChannel);
        if (it != relevantChannels.end())
        {
            std::set<channelID> usedChannels{};
            std::vector<std::pair<channelID, unsigned char>> connectionToBlock{};
            unsigned char currentTrack = std::numeric_limits<unsigned char>::max();

            useChannel(sourceChannel, p_net, channelInformation, connectionToBlock, usedChannels, currentTrack);

            // can be an adjacent block or the sourceBlock itself, if the net uses its output as input
            std::set<std::string> associatedBlockNames = it->second;

            for (std::string associatedBlockName : associatedBlockNames)
            {
                updateBlockAndRelevantChannels(sourceChannel, blocks.find(associatedBlockName)->second, relevantChannels, doublyRelevantChannels, blocks);

                numberOfPinsReached++;
                p_net->setConnection(associatedBlockName, connectionToBlock);
            }

            relevantChannels.erase(it);
            doublyRelevantChannels.erase(sourceChannel);
        }

        while (numberOfPinsReached < p_net->getConnectedBlockCount())
        {
            std::set<std::string> reachedBlocks{};
            unsigned char indexOfSink{};
            channelID sink = findPin(channelToIndex, indexToChannels, arraySize, indexOfSink, channelWidth, channelInformation, relevantChannels, doublyRelevantChannels,
                                     numberOfPinsReached, reachedBlocks, blocks);
            if (!sink.isInitialized())
                return false;

            std::vector<std::pair<channelID, unsigned char>> connectionToBlock = retrace(channelToIndex, indexToChannels, p_net, sink, indexOfSink, channelInformation);

            assert(reachedBlocks.size() != 0);
            for (std::string blockName : reachedBlocks)
                p_net->setConnection(blockName, connectionToBlock);
        }

        assert(p_net->allPinsConnected());
    }

    return true;
}