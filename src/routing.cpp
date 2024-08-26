#include <cassert>
#include "constants.hpp"
#include "channel.hpp"
#include "routing.hpp"
#include "block.hpp"
#include "logging.hpp"
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

    auto result = indexToChannels.find(index);
    if (result == indexToChannels.end())
        result = indexToChannels.emplace(index, std::set<channelID>{}).first;
    result->second.emplace(channel);
}

void chooseChannelWithPin(channelID const &chosenChannel, unsigned char indexOfChosenChannel, std::map<channelID, std::set<std::string>> &relevantChannels,
                          std::set<channelID> &doublyRelevantChannels, std::set<std::string> &reachedBlocks, std::map<std::string, std::shared_ptr<block>> const &blocks)
{
    for (std::string associatedBlockName : relevantChannels.find(chosenChannel)->second)
    {
        reachedBlocks.insert(associatedBlockName);
        std::shared_ptr<block> p_block = blocks.find(associatedBlockName)->second;
        p_block->setChannelTaken(chosenChannel);

        for (channelID openChannel : p_block->getOpenChannels())
        {
            assert(relevantChannels.contains(openChannel));
            std::set<std::string> &associatedBlocks = relevantChannels.find(openChannel)->second;
            associatedBlocks.erase(associatedBlockName);
            if (!associatedBlocks.empty())
            {
                assert(doublyRelevantChannels.contains(openChannel));
                doublyRelevantChannels.erase(openChannel);
            }
            else
                relevantChannels.erase(openChannel);
        }
    }

    relevantChannels.erase(chosenChannel);
    doublyRelevantChannels.erase(chosenChannel);
}

/* @return true, if the channel was chosen, false otherwise */
bool processChannelWithIndex(channelID channel, unsigned char indexOfChannel, unsigned char expectedIndex, std::map<channelID, unsigned char> &channelToIndex, std::map<unsigned char, std::set<channelID>> &indexToChannels,
                             unsigned char &indexOfChosenChannel, std::map<channelID, std::set<std::string>> &relevantChannels, std::set<channelID> &doublyRelevantChannels,
                             std::set<std::string> &reachedBlocks, bool &relevantChannelFound, channelID &firstRelChan, unsigned char &indexOfFirstRelChan,
                             std::map<std::string, std::shared_ptr<block>> const &blocks)
{
    if (indexOfChannel > expectedIndex)
        registerIndex(channel, expectedIndex, channelToIndex, indexToChannels);

    if (!relevantChannelFound && relevantChannels.contains(channel))
    {
        if (doublyRelevantChannels.empty())
        {
            assert(relevantChannels.find(channel)->second.size() == 1);
            indexOfChosenChannel = expectedIndex;
            return true;
        }
        else
        {
            assert(relevantChannels.find(channel)->second.size() == 1);
            firstRelChan = channel;
            indexOfFirstRelChan = expectedIndex;
            relevantChannelFound = true;
        }
    }
    return false;
}

/* @return true, if the channel was chosen, false otherwise */
bool processChannelWithoutIndex(channelID channel, unsigned char indexOfChannel, std::map<channelID, unsigned char> &channelToIndex, std::map<unsigned char, std::set<channelID>> &indexToChannels,
                                unsigned char &indexOfChosenChannel, std::map<channelID, std::set<std::string>> &relevantChannels, std::set<channelID> &doublyRelevantChannels,
                                std::set<std::string> &reachedBlocks, bool &relevantChannelFound, channelID &firstRelChan, unsigned char &indexOfFirstRelChan,
                                std::map<std::string, std::shared_ptr<block>> const &blocks)
{
    registerIndex(channel, indexOfChannel, channelToIndex, indexToChannels);

    if (doublyRelevantChannels.contains(channel))
    {
        indexOfChosenChannel = indexOfChannel;
        return true;
    }
    else if (relevantChannels.contains(channel))
    {
        if (doublyRelevantChannels.empty())
        {
            indexOfChosenChannel = indexOfChannel;
            return true;
        }
        else
        {
            assert(relevantChannels.find(channel)->second.size() == 1);
            firstRelChan = channel;
            indexOfFirstRelChan = indexOfChannel;
            relevantChannelFound = true;
        }
    }
    return false;
}

/* Finds a pin by searching neighbouring channels of the channels, that are already part of the net. In the first iteration, these are the channels with an index of zero. Updates
indexToChannels and channelToIndex to find relevant channels faster during the next call of the function. RelevantChannels and doublyRelevantChannels indicate which channels contain pins
and before returning the chosen channel, the channel is removed from both.

If a channel, that contains a pin, that needs to be connected to the net, ('relevant channel') is found, this is noted. If channels exist, that contain two pins, that need to be connected,
('doubly relevant channel') the search continues for constants::additionalIterationsForDoublyRelevantChannels iterations or until such a channel is found. If no doubly relevant channel is
found, the first relevant channel, that was found, is returned. If no doubly relevant channel exists, the search stops as soon as any relevant channel is found.
    @return if the search is successful: the chosen channel, that contains or two pins, that need to be connected to the net - if not: a channel created with the default constructor
*/
channelID findPin(std::map<channelID, unsigned char> &channelToIndex, std::map<unsigned char, std::set<channelID>> &indexToChannels, unsigned char arraySize, unsigned char &indexOfChosenChannel,
                  unsigned char channelWidth, auto const &channelInformation, std::map<channelID, std::set<std::string>> &relevantChannels, std::set<channelID> &doublyRelevantChannels,
                  std::set<std::string> &reachedBlocks, std::map<std::string, std::shared_ptr<block>> const &blocks)
{
    unsigned char currentIndex = constants::indexZero;
    std::set<channelID> fullyProcessedChannels{};
    bool relevantChannelFound{};
    channelID firstRelChan;
    unsigned char indexOfFirstRelChan{};

    assert(indexToChannels.contains(constants::indexZero));

    auto iTCEntry = indexToChannels.find(currentIndex);
    while (iTCEntry != indexToChannels.end() && (!relevantChannelFound || (!doublyRelevantChannels.empty() && currentIndex < indexOfFirstRelChan + constants::additionalIterationsForDoublyRelevantChannels)))
    {
        std::set<channelID> channelsOfCurrentIndex = iTCEntry->second;

        for (channelID channel : channelsOfCurrentIndex)
        {
            if (fullyProcessedChannels.contains(channel))
                continue;

            for (channelID neighbour : channel.getNeighbours(arraySize))
            {
                if (auto it = channelToIndex.find(neighbour); it != channelToIndex.end())
                {
                    assert(it->second >= currentIndex - 1);
                    bool chosen = processChannelWithIndex(neighbour, it->second, currentIndex + 1, channelToIndex, indexToChannels, indexOfChosenChannel, relevantChannels, doublyRelevantChannels, reachedBlocks,
                                                          relevantChannelFound, firstRelChan, indexOfFirstRelChan, blocks);
                    if (chosen)
                    {
                        chooseChannelWithPin(neighbour, indexOfChosenChannel, relevantChannels, doublyRelevantChannels, reachedBlocks, blocks);
                        return neighbour;
                    }
                }
                else if (!isChannelFull(neighbour, channelInformation, channelWidth))
                {
                    bool chosen = processChannelWithoutIndex(neighbour, currentIndex + 1, channelToIndex, indexToChannels, indexOfChosenChannel, relevantChannels, doublyRelevantChannels,
                                                             reachedBlocks, relevantChannelFound, firstRelChan, indexOfFirstRelChan, blocks);
                    if (chosen)
                    {
                        chooseChannelWithPin(neighbour, indexOfChosenChannel, relevantChannels, doublyRelevantChannels, reachedBlocks, blocks);
                        return neighbour;
                    }
                }
            }
            fullyProcessedChannels.emplace(channel);
        }
        currentIndex++;
        iTCEntry = indexToChannels.find(currentIndex);
    }

    if (relevantChannelFound)
    {
        assert(relevantChannels.find(firstRelChan)->second.size() == 1);
        chooseChannelWithPin(firstRelChan, indexOfFirstRelChan, relevantChannels, doublyRelevantChannels, reachedBlocks, blocks);
        indexOfChosenChannel = indexOfFirstRelChan;
        return firstRelChan;
    }
    else
        return constants::uninitializedChannel;
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

std::vector<std::pair<channelID, unsigned char>> retrace(auto &channelToIndex, auto &indexToChannels, std::shared_ptr<net> const &p_net, channelID channelWithPin, unsigned char indexOfChannelWithPin,
                                                         std::map<channelID, channelInfo> &channelInformation, unsigned char arraySize)
{
    assert(channelWithPin.isInitialized());

    std::set<channelID> usedChannels{};
    std::vector<std::pair<channelID, unsigned char>> connectionToBlock{};
    unsigned char currentTrack = std::numeric_limits<unsigned char>::max();

    useChannel(channelWithPin, p_net, channelInformation, connectionToBlock, usedChannels, currentTrack);

    channelID currentChannel = channelWithPin;
    unsigned char expectedIndex = indexOfChannelWithPin;

    do
    {
        expectedIndex--;

        assert(indexToChannels.contains(expectedIndex));
        channelID chosenChannel = chooseNeighbouringChannel(currentChannel, arraySize, indexToChannels.find(expectedIndex)->second, currentTrack, channelInformation);

        useChannel(chosenChannel, p_net, channelInformation, connectionToBlock, usedChannels, currentTrack);

        currentChannel = chosenChannel;
    } while (expectedIndex > 0);

    for (channelID channel : usedChannels)
        registerIndex(channel, constants::indexZero, channelToIndex, indexToChannels);

    return connectionToBlock;
}

/* @return The amount of nets, that were processed successfully. */
unsigned short routeNets(unsigned char arraySize, unsigned char channelWidth, std::vector<std::shared_ptr<net>> const &sortedNets, std::map<std::string, std::shared_ptr<block>> const &blocks)
{
    std::map<channelID, channelInfo> channelInformation = generateChannelInformation(arraySize);

    for (unsigned short index = 0; index < sortedNets.size(); index++)
    {
        std::shared_ptr<net> p_net = sortedNets[index];

        channelID sourceChannel = p_net->getSourceChannel();
        if (isChannelFull(sourceChannel, channelInformation, channelWidth))
        {
            printLogMessage("Routing failed due to the source channel of block '" + p_net->getSourceBlockName() + "' being full.");
            return index;
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

        if (relevantChannels.contains(sourceChannel))
        {
            std::set<std::string> reachedBlocks{};
            chooseChannelWithPin(sourceChannel, constants::indexZero, relevantChannels, doublyRelevantChannels, reachedBlocks, blocks);

            std::set<channelID> usedChannels{};
            std::vector<std::pair<channelID, unsigned char>> connectionToBlock{};
            unsigned char currentTrack = std::numeric_limits<unsigned char>::max();

            useChannel(sourceChannel, p_net, channelInformation, connectionToBlock, usedChannels, currentTrack);

            // can be an adjacent block or the sourceBlock itself, if the net uses its output as input
            numberOfPinsReached += reachedBlocks.size();
            for (std::string reachedBlock : reachedBlocks)
                p_net->setConnection(reachedBlock, connectionToBlock);
        }

        while (numberOfPinsReached < p_net->getConnectedBlockCount())
        {
            std::set<std::string> reachedBlocks{};
            unsigned char indexOfChannelWithPin{};
            channelID channelWithPin = findPin(channelToIndex, indexToChannels, arraySize, indexOfChannelWithPin, channelWidth, channelInformation, relevantChannels, doublyRelevantChannels,
                                               reachedBlocks, blocks);
            if (!channelWithPin.isInitialized())
                return index;

            std::vector<std::pair<channelID, unsigned char>> connectionToBlock = retrace(channelToIndex, indexToChannels, p_net, channelWithPin, indexOfChannelWithPin, channelInformation, arraySize);

            assert(reachedBlocks.size() != 0);
            numberOfPinsReached += reachedBlocks.size();
            for (std::string blockName : reachedBlocks)
                p_net->setConnection(blockName, connectionToBlock);
        }

        assert(p_net->allPinsConnected());
    }

    return sortedNets.size();
}