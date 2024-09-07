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

void registerIndex(channelID channel, unsigned char index, std::map<unsigned char, std::set<channelID>> &indexToChannels)
{
    auto result = indexToChannels.find(index);
    if (result == indexToChannels.end())
        result = indexToChannels.emplace(index, std::move(std::set<channelID>{})).first;
    result->second.emplace(channel);
}

/* Finds a pin by searching neighbouring channels of the channels, that are already part of the net. In the first iteration, these are the channels with an index of zero. Updates
indexToChannels to find relevant channels faster during the next call of the function. RelevantChannels and doublyRelevantChannels indicate which channels contain pins
and before returning the chosen channel, the channel is removed from both.

If a channel, that contains a pin, that needs to be connected to the net, ('relevant channel') is found, this is noted. If channels exist, that contain two pins, that need to be connected,
('doubly relevant channel') the search continues for constants::additionalIterationsForDoublyRelevantChannels iterations or until such a channel is found. If no doubly relevant channel is
found, the first relevant channel, that was found, is returned. If no doubly relevant channel exists, the search stops as soon as any relevant channel is found.
    @return if the search is successful: the chosen channel, that contains or two pins, that need to be connected to the net - if not: a channel created with the default constructor
*/
findResult findPin(unsigned char trackToUse, std::map<unsigned char, std::set<channelID>> &indexToChannels, unsigned char arraySize,
                   unsigned char channelWidth, std::map<channelID, channelInfo> const &channelInformation, std::map<channelID, std::set<std::string>> const &relevantChannels, std::set<channelID> const &doublyRelevantChannels,
                   std::map<std::string, std::shared_ptr<block>> const &blocks, unsigned char maximumIndex)
{
    bool relevantChannelFound{};
    channelID firstRelChan;
    unsigned char indexOfFirstRelChan = std::numeric_limits<unsigned char>::max();

    std::set<channelID> fullyProcessedChannels{};
    unsigned char currentIndex = constants::indexZero;

    auto iterator = indexToChannels.find(currentIndex);
    while (currentIndex < maximumIndex && iterator != indexToChannels.end() &&
           (!relevantChannelFound || (!doublyRelevantChannels.empty() && currentIndex < indexOfFirstRelChan + constants::additionalIterationsForDoublyRelevantChannels)))
    {
        for (channelID channel : iterator->second)
        {
            if (fullyProcessedChannels.contains(channel))
                continue;

            for (channelID neighbour : channel.generateNeighbours(arraySize))
            {
                if (!isChannelTrackFree(neighbour, trackToUse, channelInformation))
                    continue;

                unsigned char indexOfNeighbour = currentIndex + 1;
                registerIndex(neighbour, indexOfNeighbour, indexToChannels);

                if (doublyRelevantChannels.contains(neighbour))
                    return findResult{trackToUse, neighbour, indexOfNeighbour};
                else if (!relevantChannelFound && relevantChannels.contains(neighbour))
                {
                    if (doublyRelevantChannels.empty())
                        return findResult{trackToUse, neighbour, indexOfNeighbour};
                    else
                    {
                        firstRelChan = neighbour;
                        indexOfFirstRelChan = indexOfNeighbour;
                        relevantChannelFound = true;
                    }
                }
            }
            fullyProcessedChannels.emplace(channel);
        }
        currentIndex++;
        iterator = indexToChannels.find(currentIndex);
    }

        if (relevantChannelFound)
    {
        assert(indexOfFirstRelChan != std::numeric_limits<unsigned char>::max());
        return findResult{trackToUse, firstRelChan, indexOfFirstRelChan};
    }
    else
        return findResult{};
}

findResult findPinForGivenTracks(std::set<unsigned char> const &tracksToCheck, std::vector<std::map<unsigned char, std::set<channelID>>> &indexToChannelsVectors, unsigned char arraySize,
                                 unsigned char channelWidth, std::map<channelID, channelInfo> const &channelInformation, std::map<channelID, std::set<std::string>> const &relevantChannels,
                                 std::set<channelID> const &doublyRelevantChannels, std::map<std::string, std::shared_ptr<block>> const &blocks, unsigned char &maximumIndex)
{
    findResult bestResult;

    for (unsigned char track : tracksToCheck)
    {
        findResult result = findPin(track, indexToChannelsVectors[track], arraySize, channelWidth, channelInformation, relevantChannels,
                                    doublyRelevantChannels, blocks, maximumIndex);

        if (result.isInitialized() && result.indexOfChosenChannel < bestResult.indexOfChosenChannel)
        {
            bestResult = result;
            if (bestResult.indexOfChosenChannel < maximumIndex)
                maximumIndex = bestResult.indexOfChosenChannel;
        }
    }
    return bestResult;
}

void useChannel(channelID channel, unsigned char track, std::shared_ptr<net> const &p_net, std::map<channelID, channelInfo> &channelInformation,
                std::vector<channelID> &connectionToBlock)
{
    assert(channel.isInitialised());
    updateChannelInfo(channel, channelInformation, track);
    p_net->setUsedTrack(channel, track);
    connectionToBlock.push_back(channel);
}

std::vector<channelID> retrace(findResult result, std::map<unsigned char, std::set<channelID>> &indexToChannels,
                               std::shared_ptr<net> const &p_net, std::map<channelID, channelInfo> &channelInformation, unsigned char arraySize, bool previouslyUsedTrack)
{
    assert(result.isInitialized());

    std::vector<channelID> connectionToBlock{};

    useChannel(result.chosenChannel, result.track, p_net, channelInformation, connectionToBlock);

    channelID currentChannel = result.chosenChannel;
    int expectedIndex = result.indexOfChosenChannel - 1;

    while (expectedIndex >= 1)
    {
        channelID chosenChannel = chooseNeighbouringChannel(currentChannel, arraySize, indexToChannels.find(expectedIndex)->second, channelInformation);

        useChannel(chosenChannel, result.track, p_net, channelInformation, connectionToBlock);

        currentChannel = chosenChannel;
        expectedIndex--;
    }

    std::set<channelID> &zeroIndexSet = indexToChannels.find(constants::indexZero)->second;
    channelID chosenChannelWithIndexZero = chooseNeighbouringChannel(currentChannel, arraySize, zeroIndexSet, channelInformation);

    if (previouslyUsedTrack)
        connectionToBlock.push_back(chosenChannelWithIndexZero);
    else
        useChannel(chosenChannelWithIndexZero, result.track, p_net, channelInformation, connectionToBlock);

    for (channelID channel : connectionToBlock)
        zeroIndexSet.insert(channel);

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

        // indexToChannels can contain multiple entries for a channel - only the lowest is of relevance
        std::vector<std::map<unsigned char, std::set<channelID>>> indexToChannelsVectors{};
        indexToChannelsVectors.reserve(channelWidth);
        // register sourceChannel index
        for (unsigned char track = 0; track < channelWidth; track++)
            indexToChannelsVectors.push_back(std::move(std::map<unsigned char, std::set<channelID>>{{constants::indexZero, std::move(std::set<channelID>{sourceChannel})}}));

        if (relevantChannels.contains(sourceChannel))
        {
            unsigned char optimalTrack = findOptimalTrack(sourceChannel, channelInformation, channelWidth);

            std::vector<channelID> connectionToBlock{};
            useChannel(sourceChannel, optimalTrack, p_net, channelInformation, connectionToBlock);

            updateNetAndCounterAndRelevantChannels(sourceChannel, optimalTrack, connectionToBlock, blocks, p_net, blocksReached, relevantChannels, doublyRelevantChannels);
        }

        while (blocksReached < p_net->getConnectedBlockCount())
        {
            bool previouslyUsedTrack = false;
            unsigned char maximumIndex = std::numeric_limits<unsigned char>::max();

            findResult bestResult = findPinForGivenTracks(p_net->findUsedTracksAtSourceChannel(), indexToChannelsVectors, arraySize, channelWidth, channelInformation,
                                                          relevantChannels, doublyRelevantChannels, blocks, maximumIndex);

            // new tracks need to find a solution that is at least twice as good (connection half as long), to be chosen
            if (bestResult.isInitialized())
                maximumIndex /= 2;

            findResult bestResultNewTrack;
            if (maximumIndex > constants::indexZero)
                bestResultNewTrack = findPinForGivenTracks(generateFreeTracks(sourceChannel, channelInformation, channelWidth), indexToChannelsVectors, arraySize,
                                                           channelWidth, channelInformation, relevantChannels, doublyRelevantChannels, blocks, maximumIndex);

            if (!bestResult.isInitialized() && !bestResultNewTrack.isInitialized())
                return netIndex;
            else if (!bestResult.isInitialized() || constants::ratioNewToOld * bestResultNewTrack.indexOfChosenChannel < bestResult.indexOfChosenChannel)
            {
                previouslyUsedTrack = false;
                bestResult = bestResultNewTrack;
            }
            else
                previouslyUsedTrack = true;

            std::vector<channelID> connectionToBlock = retrace(bestResult, indexToChannelsVectors[bestResult.track], p_net, channelInformation, arraySize, previouslyUsedTrack);

            updateNetAndCounterAndRelevantChannels(bestResult.chosenChannel, bestResult.track, connectionToBlock, blocks, p_net, blocksReached, relevantChannels, doublyRelevantChannels);

            // only keep index zero
            for (int track = 0; track < indexToChannelsVectors.size(); track++)
                indexToChannelsVectors[track] = std::map<unsigned char, std::set<channelID>>{{constants::indexZero, std::move(indexToChannelsVectors[track].find(constants::indexZero)->second)}};
        }

        assert(p_net->allPinsConnected());
    }

    return nets.size();
}