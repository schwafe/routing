#include <cassert>
#include "constants.hpp"
#include "routing.hpp"
#include "block.hpp"
#include "logging.hpp"
#include "net.hpp"
#include "channel/channel.hpp"
#include "findResult.hpp"
#include "concurrentRouting.hpp"

void generateRelevantChannels(std::map<channelID, std::set<std::string>> &relevantChannels, std::set<channelID> &doublyRelevantChannels,
                              std::shared_ptr<net> const &p_net, std::map<std::string, std::shared_ptr<block>> const &blocks)
{
    for (std::string const &connectedBlockName : p_net->getNamesOfConnectedBlocks())
    {
        assert(blocks.contains(connectedBlockName));
        for (channelID channelWithPin : blocks.find(connectedBlockName)->second->getOpenPins())
        {
            if (auto it = relevantChannels.find(channelWithPin); it != relevantChannels.end())
            {
                it->second.insert(connectedBlockName);
                assert(relevantChannels.find(channelWithPin)->second.size() == 2);

                assert(!doublyRelevantChannels.contains(channelWithPin));
                doublyRelevantChannels.insert(channelWithPin);
            }
            else
                relevantChannels.emplace(channelWithPin, std::move(std::set<std::string>{connectedBlockName}));
        }
    }
}

/* Adds or overwrites the entry in channelToIndex with the given index and adds the channel to the set of channels for the given index in indexToChannels.
Other entries for the same channel may exist in indexToChannels. */
void registerIndex(channelID channel, unsigned char index, std::map<channelID, unsigned char> &channelToIndex, std::map<unsigned char, std::set<channelID>> &indexToChannels)
{
    assert(!channelToIndex.contains(channel) || channelToIndex.find(channel)->second > index || (channelToIndex.find(channel)->second == constants::indexZero && index == constants::indexZero));
    channelToIndex.insert_or_assign(channel, index);

    auto result = indexToChannels.find(index);
    if (result == indexToChannels.end())
        result = indexToChannels.emplace(index, std::move(std::set<channelID>{})).first;
    result->second.emplace(channel);
}

void registerSourceChannelIndex(channelID sourceChannel, std::map<unsigned char, std::map<channelID, unsigned char>> &channelToIndexMaps,
                                std::map<unsigned char, std::map<unsigned char, std::set<channelID>>> &indexToChannelsMaps, unsigned char channelWidth)
{
    for (unsigned char track = 0; track < channelWidth; track++)
    {
        channelToIndexMaps.emplace(track, std::move(std::map<channelID, unsigned char>{{sourceChannel, constants::indexZero}}));
        assert(channelToIndexMaps.find(track)->second.size() == 1);

        indexToChannelsMaps.emplace(track, std::move(std::map<unsigned char, std::set<channelID>>{{constants::indexZero, std::move(std::set<channelID>{sourceChannel})}}));
        assert(indexToChannelsMaps.find(track)->second.contains(constants::indexZero));
        assert(indexToChannelsMaps.find(track)->second.find(constants::indexZero)->second.size() == 1);
    }
    assert(channelToIndexMaps.size() == channelWidth);
    assert(indexToChannelsMaps.size() == channelWidth);
}

void updateNetAndCounterAndRelevantChannels(channelID const &chosenChannel, unsigned char track, std::vector<channelID> const &connectionToBlock,
                                            std::map<std::string, std::shared_ptr<block>> const &blocks, std::shared_ptr<net> const &p_net, unsigned short &blocksReached,
                                            std::map<channelID, std::set<std::string>> &relevantChannels, std::set<channelID> &doublyRelevantChannels)
{
    for (std::string const &associatedBlockName : relevantChannels.find(chosenChannel)->second)
    {
        blocksReached++;
        p_net->setConnection(associatedBlockName, track, connectionToBlock);

        std::shared_ptr<block> const &p_block = blocks.find(associatedBlockName)->second;
        p_block->setPinTaken(chosenChannel);

        for (channelID openChannel : p_block->getOpenPins())
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
bool handleChannelWithIndex(channelID channel, unsigned char indexOfChannel, unsigned char expectedIndex, unsigned char track, std::map<channelID, unsigned char> &channelToIndex, std::map<unsigned char, std::set<channelID>> &indexToChannels,
                            unsigned char &indexOfChosenChannel, std::map<channelID, std::set<std::string>> const &relevantChannels, std::set<channelID> const &doublyRelevantChannels,
                            bool &relevantChannelFound, channelID &firstRelChan, unsigned char &indexOfFirstRelChan, std::map<std::string, std::shared_ptr<block>> const &blocks,
                            std::map<channelID, channelInfo> const &channelInformation)
{
    assert(expectedIndex > 0);
    unsigned char index = indexOfChannel;
    if (indexOfChannel > expectedIndex)
    {
        registerIndex(channel, expectedIndex, channelToIndex, indexToChannels);
        index = expectedIndex;
    }

    if (doublyRelevantChannels.contains(channel))
    {
        assert(relevantChannels.find(channel)->second.size() == 2);
        indexOfChosenChannel = index;
        return true;
    }
    else if (!relevantChannelFound && relevantChannels.contains(channel))
    {
        assert(relevantChannels.find(channel)->second.size() == 1);
        if (doublyRelevantChannels.empty())
        {
            indexOfChosenChannel = index;
            return true;
        }
        else
        {
            firstRelChan = channel;
            indexOfFirstRelChan = index;
            relevantChannelFound = true;
        }
    }
    return false;
}

/* @return true, if the channel was chosen, false otherwise */
bool handleChannelWithoutIndex(channelID channel, unsigned char indexOfChannel, unsigned char track, std::map<channelID, unsigned char> &channelToIndex, std::map<unsigned char, std::set<channelID>> &indexToChannels,
                               unsigned char &indexOfChosenChannel, std::map<channelID, std::set<std::string>> const &relevantChannels, std::set<channelID> const &doublyRelevantChannels,
                               bool &relevantChannelFound, channelID &firstRelChan, unsigned char &indexOfFirstRelChan, std::map<std::string, std::shared_ptr<block>> const &blocks)
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
findResult findPin(unsigned char trackToUse, std::map<channelID, unsigned char> &channelToIndex, std::map<unsigned char, std::set<channelID>> &indexToChannels, unsigned char arraySize,
                   unsigned char channelWidth, std::map<channelID, channelInfo> const &channelInformation, std::map<channelID, std::set<std::string>> const &relevantChannels, std::set<channelID> const &doublyRelevantChannels,
                   std::map<std::string, std::shared_ptr<block>> const &blocks, unsigned char maximumIndex)
{
    unsigned char currentIndex = constants::indexZero;
    std::set<channelID> fullyProcessedChannels{};
    unsigned char indexOfChosenChannel{};

    bool relevantChannelFound{};
    channelID firstRelChan;
    unsigned char indexOfFirstRelChan{};

    assert(indexToChannels.contains(constants::indexZero));

    auto iTCEntry = indexToChannels.find(currentIndex);
    while (currentIndex < maximumIndex && iTCEntry != indexToChannels.end() &&
           (!relevantChannelFound || (!doublyRelevantChannels.empty() && currentIndex < indexOfFirstRelChan + constants::additionalIterationsForDoublyRelevantChannels)))
    {
        std::set<channelID> const &channelsOfCurrentIndex = iTCEntry->second;

        for (channelID channel : channelsOfCurrentIndex)
        {
            if (fullyProcessedChannels.contains(channel))
                continue;

            for (channelID neighbour : channel.getNeighbours(arraySize))
            {
                if (auto it = channelToIndex.find(neighbour); it != channelToIndex.end())
                {
                    assert(it->second >= currentIndex - 1);
                    bool chosen = handleChannelWithIndex(neighbour, it->second, currentIndex + 1, trackToUse, channelToIndex, indexToChannels, indexOfChosenChannel, relevantChannels, doublyRelevantChannels,
                                                         relevantChannelFound, firstRelChan, indexOfFirstRelChan, blocks, channelInformation);
                    if (chosen)
                        return findResult{trackToUse, neighbour, indexOfChosenChannel};
                }
                else if (isChannelTrackFree(neighbour, trackToUse, channelInformation))
                {
                    bool chosen = handleChannelWithoutIndex(neighbour, currentIndex + 1, trackToUse, channelToIndex, indexToChannels, indexOfChosenChannel, relevantChannels, doublyRelevantChannels,
                                                            relevantChannelFound, firstRelChan, indexOfFirstRelChan, blocks);
                    if (chosen)
                        return findResult{trackToUse, neighbour, indexOfChosenChannel};
                }
            }
            fullyProcessedChannels.emplace(channel);
        }
        currentIndex++;
        iTCEntry = indexToChannels.find(currentIndex);
    }

    if (relevantChannelFound)
        return findResult{trackToUse, firstRelChan, indexOfFirstRelChan};
    else
        return findResult{};
}

findResult findPinForGivenTracks(std::set<unsigned char> const &tracksToCheck, std::map<unsigned char, std::map<channelID, unsigned char>> &channelToIndexMaps, std::map<unsigned char, std::map<unsigned char, std::set<channelID>>> &indexToChannelsMaps, unsigned char arraySize,
                                 unsigned char channelWidth, std::map<channelID, channelInfo> const &channelInformation, std::map<channelID, std::set<std::string>> const &relevantChannels,
                                 std::set<channelID> const &doublyRelevantChannels, std::map<std::string, std::shared_ptr<block>> const &blocks)
{
    findResult bestResult;
    unsigned char bestIndex = std::numeric_limits<unsigned char>::max();

    for (unsigned char track : tracksToCheck)
    {
        findResult result = findPin(track, channelToIndexMaps.find(track)->second, indexToChannelsMaps.find(track)->second, arraySize, channelWidth, channelInformation, relevantChannels,
                                    doublyRelevantChannels, blocks, bestIndex);

        if (result.chosenChannel.isInitialized() && result.indexOfChosenChannel < bestIndex)
        {
            bestIndex = result.indexOfChosenChannel;
            bestResult = result;
        }
    }
    return bestResult;
}

void updateChannelAndNetAndConnection(channelID channel, unsigned char track, std::shared_ptr<net> const &p_net, std::map<channelID, channelInfo> &channelInformation,
                                      std::vector<channelID> &connectionToBlock)
{
    useChannel(channel, channelInformation, track);
    p_net->setUsedTrack(channel, track);
    connectionToBlock.push_back(channel);
}

std::vector<channelID> retrace(findResult result, std::map<channelID, unsigned char> &channelToIndex, std::map<unsigned char, std::set<channelID>> &indexToChannels,
                               std::shared_ptr<net> const &p_net, std::map<channelID, channelInfo> &channelInformation, unsigned char arraySize, bool previouslyUsedTrack)
{
    assert(result.chosenChannel.isInitialized());

    std::vector<channelID> connectionToBlock{};

    updateChannelAndNetAndConnection(result.chosenChannel, result.track, p_net, channelInformation, connectionToBlock);

    channelID currentChannel = result.chosenChannel;
    int expectedIndex = result.indexOfChosenChannel - 1;

    while (expectedIndex >= 1)
    {
        assert(indexToChannels.contains(expectedIndex));
        channelID chosenChannel = chooseNeighbouringChannel(currentChannel, arraySize, indexToChannels.find(expectedIndex)->second, result.track, channelInformation);

        updateChannelAndNetAndConnection(chosenChannel, result.track, p_net, channelInformation, connectionToBlock);

        currentChannel = chosenChannel;
        expectedIndex--;
    }

    if (previouslyUsedTrack)
    {
        channelID chosenChannel = chooseNeighbouringChannelWithUsedTrack(currentChannel, arraySize, indexToChannels.find(constants::indexZero)->second, channelInformation);
        connectionToBlock.push_back(chosenChannel);
    }
    else
    {
        channelID chosenChannel = chooseNeighbouringChannel(currentChannel, arraySize, indexToChannels.find(expectedIndex)->second, result.track, channelInformation);
        updateChannelAndNetAndConnection(chosenChannel, result.track, p_net, channelInformation, connectionToBlock);
    }

    for (channelID channel : connectionToBlock)
        registerIndex(channel, constants::indexZero, channelToIndex, indexToChannels);

    return std::move(connectionToBlock);
}

/* @return The amount of nets, that were processed successfully. */
unsigned short routeNets(unsigned char arraySize, unsigned char channelWidth, std::vector<std::shared_ptr<net>> const &nets, std::map<std::string, std::shared_ptr<block>> const &blocks)
{
    std::map<channelID, channelInfo> channelInformation = generateChannelInformation(arraySize);

    for (std::size_t netIndex = 0; netIndex < nets.size(); netIndex++)
    {
        std::shared_ptr<net> const &p_net = nets[netIndex];
        channelID sourceChannel = p_net->getSourceChannel();
        unsigned short blocksReached = 0;

        if (isChannelFull(sourceChannel, channelInformation, channelWidth))
            return netIndex;

        /* stores channels, that contain input pins of two blocks, that are to be connected to the net*/
        std::set<channelID> doublyRelevantChannels{};
        /* stores channels, that contain pins of blocks, that are to be connected to the net and for each channel the names of the blocks */
        std::map<channelID, std::set<std::string>> relevantChannels{};

        generateRelevantChannels(relevantChannels, doublyRelevantChannels, p_net, blocks);

        std::map<unsigned char, std::map<channelID, unsigned char>> channelToIndexMaps{};
        // indexToChannels can contain multiple entries for a channel - only the lowest is of relevance
        std::map<unsigned char, std::map<unsigned char, std::set<channelID>>> indexToChannelsMaps{};

        registerSourceChannelIndex(sourceChannel, channelToIndexMaps, indexToChannelsMaps, channelWidth);

        if (relevantChannels.contains(sourceChannel))
        {
            unsigned char optimalTrack = findOptimalTrack(sourceChannel, channelInformation, channelWidth);

            std::vector<channelID> connectionToBlock{};
            updateChannelAndNetAndConnection(sourceChannel, optimalTrack, p_net, channelInformation, connectionToBlock);

            updateNetAndCounterAndRelevantChannels(sourceChannel, optimalTrack, connectionToBlock, blocks, p_net, blocksReached, relevantChannels, doublyRelevantChannels);
        }

        while (blocksReached < p_net->getConnectedBlockCount())
        {
            bool previouslyUsedTrack = false;
            findResult bestResult = findPinForGivenTracks(p_net->findUsedTracksAtSourceChannel(), channelToIndexMaps, indexToChannelsMaps, arraySize, channelWidth, channelInformation,
                                                          relevantChannels, doublyRelevantChannels, blocks);

            std::set<unsigned char> freeTracks = getFreeTracks(sourceChannel, channelInformation, channelWidth);
            findResult bestResultNewTrack = findPinForGivenTracks(freeTracks, channelToIndexMaps, indexToChannelsMaps, arraySize,
                                                                  channelWidth, channelInformation, relevantChannels, doublyRelevantChannels, blocks);

            if (!bestResult.chosenChannel.isInitialized() && !bestResultNewTrack.chosenChannel.isInitialized())
                return netIndex;
            else if (bestResultNewTrack.indexOfChosenChannel * constants::ratioNewToOld < bestResult.indexOfChosenChannel)
            {
                previouslyUsedTrack = false;
                bestResult = bestResultNewTrack;
            }
            else
                previouslyUsedTrack = true;

            std::vector<channelID> connectionToBlock = retrace(bestResult, channelToIndexMaps.find(bestResult.track)->second,
                                                               indexToChannelsMaps.find(bestResult.track)->second, p_net, channelInformation, arraySize, previouslyUsedTrack);

            updateNetAndCounterAndRelevantChannels(bestResult.chosenChannel, bestResult.track, connectionToBlock, blocks, p_net, blocksReached, relevantChannels, doublyRelevantChannels);
        }

        assert(p_net->allPinsConnected());
    }

    return nets.size();
}