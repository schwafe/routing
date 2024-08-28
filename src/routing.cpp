#include <cassert>
#include "constants.hpp"
#include "routing.hpp"
#include "block.hpp"
#include "logging.hpp"
#include "net.hpp"
#include "channel/channel.hpp"

std::map<channelID, std::set<std::string>> findRelevantChannels(std::map<channelID, std::set<std::string>> &relevantChannels, std::set<channelID> &doublyRelevantChannels,
                                                                std::shared_ptr<net> const &p_net, std::map<std::string, std::shared_ptr<block>> const &blocks)
{
    for (std::string connectedBlockName : p_net->getNamesOfConnectedBlocks())
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

void registerSourceChannelIndex(channelID sourceChannel, std::map<unsigned char, std::map<channelID, unsigned char>> &channelToIndex,
                                std::map<unsigned char, std::map<unsigned char, std::set<channelID>>> &indexToChannels, unsigned char channelWidth)
{
    for (unsigned char track = 0; track < channelWidth; track++)
    {
        channelToIndex.emplace(track, std::map<channelID, unsigned char>{{sourceChannel, constants::indexZero}});
        assert(channelToIndex.find(track)->second.size() == 1 && channelToIndex.find(track)->second.find(sourceChannel)->second == constants::indexZero);

        indexToChannels.emplace(track, std::map<unsigned char, std::set<channelID>>{{constants::indexZero, std::set<channelID>{sourceChannel}}});
        assert(indexToChannels.find(track)->second.contains(constants::indexZero));
        assert(indexToChannels.find(track)->second.find(constants::indexZero)->second.size() == 1);
    }
    assert(channelToIndex.size() == channelWidth);
    assert(indexToChannels.size() == channelWidth);
}

void updateRelevantChannels(channelID const &chosenChannel, unsigned char indexOfChosenChannel, std::map<channelID, std::set<std::string>> &relevantChannels,
                            std::set<channelID> &doublyRelevantChannels, std::set<std::string> &reachedBlocks, std::map<std::string, std::shared_ptr<block>> const &blocks)
{
    for (std::string associatedBlockName : relevantChannels.find(chosenChannel)->second)
    {
        reachedBlocks.insert(associatedBlockName);
        std::shared_ptr<block> p_block = blocks.find(associatedBlockName)->second;
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
bool processChannelWithIndex(channelID channel, unsigned char indexOfChannel, unsigned char expectedIndex, unsigned char track, std::map<channelID, unsigned char> &channelToIndex, std::map<unsigned char, std::set<channelID>> &indexToChannels,
                             unsigned char &indexOfChosenChannel, std::map<channelID, std::set<std::string>> const &relevantChannels, std::set<channelID> const &doublyRelevantChannels,
                             std::set<std::string> &reachedBlocks, bool &relevantChannelFound, channelID &firstRelChan, unsigned char &indexOfFirstRelChan,
                             std::map<std::string, std::shared_ptr<block>> const &blocks, std::map<channelID, channelInfo> const &channelInformation)
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
        if (doublyRelevantChannels.empty())
        {
            assert(relevantChannels.find(channel)->second.size() == 1);
            indexOfChosenChannel = index;
            return true;
        }
        else
        {
            assert(relevantChannels.find(channel)->second.size() == 1);
            firstRelChan = channel;
            indexOfFirstRelChan = index;
            relevantChannelFound = true;
        }
    }
    return false;
}

/* @return true, if the channel was chosen, false otherwise */
bool processChannelWithoutIndex(channelID channel, unsigned char indexOfChannel, unsigned char track, std::map<channelID, unsigned char> &channelToIndex, std::map<unsigned char, std::set<channelID>> &indexToChannels,
                                unsigned char &indexOfChosenChannel, std::map<channelID, std::set<std::string>> const &relevantChannels, std::set<channelID> const &doublyRelevantChannels,
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
channelID findPin(unsigned char trackToUse, std::map<channelID, unsigned char> &channelToIndex, std::map<unsigned char, std::set<channelID>> &indexToChannels, unsigned char arraySize, unsigned char &indexOfChosenChannel,
                  unsigned char channelWidth, std::map<channelID, channelInfo> const &channelInformation, std::map<channelID, std::set<std::string>> const &relevantChannels, std::set<channelID> const &doublyRelevantChannels,
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
                    bool chosen = processChannelWithIndex(neighbour, it->second, currentIndex + 1, trackToUse, channelToIndex, indexToChannels, indexOfChosenChannel, relevantChannels, doublyRelevantChannels, reachedBlocks,
                                                          relevantChannelFound, firstRelChan, indexOfFirstRelChan, blocks, channelInformation);
                    if (chosen)
                        return neighbour;
                }
                else if (isChannelTrackFree(neighbour, trackToUse, channelInformation))
                {
                    bool chosen = processChannelWithoutIndex(neighbour, currentIndex + 1, trackToUse, channelToIndex, indexToChannels, indexOfChosenChannel, relevantChannels, doublyRelevantChannels,
                                                             reachedBlocks, relevantChannelFound, firstRelChan, indexOfFirstRelChan, blocks);
                    if (chosen)
                        return neighbour;
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
        indexOfChosenChannel = indexOfFirstRelChan;
        return firstRelChan;
    }
    else
        return constants::uninitializedChannel;
}

void findPinCC(unsigned char trackToUse, unsigned char channelWidth, unsigned char arraySize, std::map<channelID, channelInfo> const &channelInformation,
               std::map<channelID, std::set<std::string>> const &relevantChannels, std::set<channelID> const &doublyRelevantChannels,
               std::map<std::string, std::shared_ptr<block>> const &blocks, std::map<channelID, unsigned char> &channelToIndex,
               std::map<unsigned char, std::set<channelID>> &indexToChannels, channelID &chosenChannel, unsigned char &indexOfChosenChannel, std::set<std::string> &reachedBlocks)
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
                    bool chosen = processChannelWithIndex(neighbour, it->second, currentIndex + 1, trackToUse, channelToIndex, indexToChannels, indexOfChosenChannel, relevantChannels, doublyRelevantChannels, reachedBlocks,
                                                          relevantChannelFound, firstRelChan, indexOfFirstRelChan, blocks, channelInformation);
                    if (chosen)
                        chosenChannel = neighbour;
                }
                else if (isChannelTrackFree(neighbour, trackToUse, channelInformation))
                {
                    bool chosen = processChannelWithoutIndex(neighbour, currentIndex + 1, trackToUse, channelToIndex, indexToChannels, indexOfChosenChannel, relevantChannels, doublyRelevantChannels,
                                                             reachedBlocks, relevantChannelFound, firstRelChan, indexOfFirstRelChan, blocks);
                    if (chosen)
                        chosenChannel = neighbour;
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
        indexOfChosenChannel = indexOfFirstRelChan;
        chosenChannel = firstRelChan;
    }
    else
        chosenChannel = constants::uninitializedChannel;
}

void useChannel(channelID channel, unsigned char track, std::shared_ptr<net> const &p_net, std::map<channelID, channelInfo> &channelInformation,
                std::vector<channelID> &connectionToBlock)
{
    useChannel(channel, channelInformation, track);
    p_net->setUsedTrack(channel, track);

    connectionToBlock.push_back(channel);
}

std::vector<channelID> retrace(channelID channelWithPin, unsigned char indexOfChannelWithPin, unsigned char track, auto &channelToIndex, auto &indexToChannels,
                               std::shared_ptr<net> const &p_net, std::map<channelID, channelInfo> &channelInformation, unsigned char arraySize, bool previouslyUsedTrack)
{
    assert(channelWithPin.isInitialized());

    std::vector<channelID> connectionToBlock{};

    useChannel(channelWithPin, track, p_net, channelInformation, connectionToBlock);

    channelID currentChannel = channelWithPin;
    int expectedIndex = indexOfChannelWithPin - 1;

    while (expectedIndex >= 1 || (!previouslyUsedTrack && expectedIndex >= 0))
    {
        assert(indexToChannels.contains(expectedIndex));
        channelID chosenChannel = chooseNeighbouringChannel(currentChannel, arraySize, indexToChannels.find(expectedIndex)->second, track, channelInformation);

        useChannel(chosenChannel, track, p_net, channelInformation, connectionToBlock);

        currentChannel = chosenChannel;
        expectedIndex--;
    }

    if (previouslyUsedTrack)
    {
        channelID chosenChannel = chooseNeighbouringChannelWithUsedTrack(currentChannel, arraySize, indexToChannels.find(constants::indexZero)->second, channelInformation);
        connectionToBlock.push_back(chosenChannel);
    }

    for (channelID channel : connectionToBlock)
        registerIndex(channel, constants::indexZero, channelToIndex, indexToChannels);

    return connectionToBlock;
}

/* @return The amount of nets, that were processed successfully. */
unsigned short routeNets(unsigned char arraySize, unsigned char channelWidth, std::vector<std::shared_ptr<net>> const &nets, std::map<std::string, std::shared_ptr<block>> const &blocks)
{
    std::map<channelID, channelInfo> channelInformation = generateChannelInformation(arraySize);

    for (unsigned short index = 0; index < nets.size(); index++)
    {
        std::shared_ptr<net> p_net = nets[index];

        /* printLogMessage("Net " + p_net->getName() + " with blocks to connect: " + listConnectedBlocks(p_net)); */

        channelID sourceChannel = p_net->getSourceChannel();
        if (isChannelFull(sourceChannel, channelInformation, channelWidth))
            return index;

        /* stores channels, that contain input pins of two blocks, that are to be connected to the net*/
        std::set<channelID> doublyRelevantChannels{};
        /* stores channels, that contain pins of blocks, that are to be connected to the net and for each channel the names of the blocks */
        std::map<channelID, std::set<std::string>> relevantChannels{};
        findRelevantChannels(relevantChannels, doublyRelevantChannels, p_net, blocks);

        std::map<unsigned char, std::map<channelID, unsigned char>> channelToIndexMaps{};

        // can contain multiple entries for a channel - only the lowest is of relevance
        std::map<unsigned char, std::map<unsigned char, std::set<channelID>>> indexToChannelsMaps{};
        unsigned short numberOfPinsReached{};

        registerSourceChannelIndex(sourceChannel, channelToIndexMaps, indexToChannelsMaps, channelWidth);

        if (relevantChannels.contains(sourceChannel))
        {
            std::set<std::string> reachedBlocks{};
            updateRelevantChannels(sourceChannel, constants::indexZero, relevantChannels, doublyRelevantChannels, reachedBlocks, blocks);

            std::vector<channelID> connectionToBlock{};

            unsigned char optimalTrack = findOptimalTrack(sourceChannel, channelInformation, channelWidth);
            useChannel(sourceChannel, optimalTrack, p_net, channelInformation, connectionToBlock);

            // can be an adjacent block or the sourceBlock itself, if the net uses its output as input
            numberOfPinsReached += reachedBlocks.size();
            for (std::string reachedBlock : reachedBlocks)
                p_net->setConnection(reachedBlock, optimalTrack, connectionToBlock);
        }

        while (numberOfPinsReached < p_net->getConnectedBlockCount())
        {
            std::set<std::string> bestReachedBlocks;
            bool previouslyUsedTrack{};

            channelID bestChannelWithPin;
            unsigned char bestTrack = std::numeric_limits<unsigned char>::max();
            unsigned char bestIndexOfChannelWithPin = std::numeric_limits<unsigned char>::max();
            std::set<unsigned char> previouslyUsedTracks = p_net->getUsedTracksAtSourceChannel();

            for (unsigned char track : previouslyUsedTracks)
            {
                std::set<std::string> reachedBlocks{};
                std::map<channelID, unsigned char> channelToIndex = channelToIndexMaps.find(track)->second;
                std::map<unsigned char, std::set<channelID>> indexToChannels = indexToChannelsMaps.find(track)->second;
                unsigned char indexOfChannelWithPin{};

                channelID channelWithPin = findPin(track, channelToIndex, indexToChannels, arraySize, indexOfChannelWithPin, channelWidth, channelInformation, relevantChannels, doublyRelevantChannels,
                                                   reachedBlocks, blocks);

                /*                 printLogMessage("ChannelToIndex for track " + std::to_string(track));
                                printChannelToIndex(channelToIndex); */

                channelToIndexMaps.insert_or_assign(track, channelToIndex);
                indexToChannelsMaps.insert_or_assign(track, indexToChannels);

                if (channelWithPin.isInitialized() && indexOfChannelWithPin < bestIndexOfChannelWithPin)
                {
                    bestTrack = track;
                    bestIndexOfChannelWithPin = indexOfChannelWithPin;
                    bestChannelWithPin = channelWithPin;
                    bestReachedBlocks = reachedBlocks;
                    previouslyUsedTrack = true;
                }
            }

            if (!bestChannelWithPin.isInitialized())
            {
                assert(bestTrack == std::numeric_limits<unsigned char>::max());
                assert(bestIndexOfChannelWithPin == std::numeric_limits<unsigned char>::max());

                for (unsigned char track : getFreeTracks(sourceChannel, channelInformation, channelWidth))
                {
                    assert(!previouslyUsedTracks.contains(track));

                    std::set<std::string> reachedBlocks{};
                    std::map<channelID, unsigned char> channelToIndex = channelToIndexMaps.find(track)->second;
                    std::map<unsigned char, std::set<channelID>> indexToChannels = indexToChannelsMaps.find(track)->second;
                    unsigned char indexOfChannelWithPin{};

                    channelID channelWithPin = findPin(track, channelToIndex, indexToChannels, arraySize, indexOfChannelWithPin, channelWidth, channelInformation, relevantChannels, doublyRelevantChannels,
                                                       reachedBlocks, blocks);

                    /*                     printLogMessage("Channel: " + channelIDToString(channelWithPin) + " index: " + std::to_string(indexOfChannelWithPin));
                                        printLogMessage("ChannelToIndex for track " + std::to_string(track));
                                        printChannelToIndex(channelToIndex); */

                    channelToIndexMaps.insert_or_assign(track, channelToIndex);
                    indexToChannelsMaps.insert_or_assign(track, indexToChannels);

                    if (channelWithPin.isInitialized() && indexOfChannelWithPin < bestIndexOfChannelWithPin)
                    {
                        bestTrack = track;
                        bestIndexOfChannelWithPin = indexOfChannelWithPin;
                        bestChannelWithPin = channelWithPin;
                        bestReachedBlocks = reachedBlocks;
                        previouslyUsedTrack = false;
                    }
                }
            }

            if (!bestChannelWithPin.isInitialized())
                return index;

            assert(bestTrack != std::numeric_limits<unsigned char>::max());
            assert(bestIndexOfChannelWithPin != std::numeric_limits<unsigned char>::max());

            /* printLogMessage("Best track: " + std::to_string(bestTrack) + " bestChannel " + channelIDToString(bestChannelWithPin) + " bestIndex " + std::to_string(bestIndexOfChannelWithPin)); */

            std::vector<channelID> connectionToBlock = retrace(bestChannelWithPin, bestIndexOfChannelWithPin, bestTrack, channelToIndexMaps.find(bestTrack)->second,
                                                               indexToChannelsMaps.find(bestTrack)->second, p_net, channelInformation, arraySize, previouslyUsedTrack);

            updateRelevantChannels(bestChannelWithPin, bestIndexOfChannelWithPin, relevantChannels, doublyRelevantChannels, bestReachedBlocks, blocks);

            assert(bestReachedBlocks.size() != 0);
            numberOfPinsReached += bestReachedBlocks.size();
            for (std::string blockName : bestReachedBlocks)
            {
                p_net->setConnection(blockName, bestTrack, connectionToBlock);
                /* printLogMessage("Block connected: " + blockName); */
            }
        }

        assert(p_net->allPinsConnected());
    }

    return nets.size();
}