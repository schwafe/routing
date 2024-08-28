#include "concurrentRouting.hpp"
#include "routing.hpp"

#include <cassert>
#include <thread>
#include <future>

void findPin(unsigned char trackToUse, unsigned char channelWidth, unsigned char arraySize, std::map<channelID, unsigned char> cTI,
               std::map<unsigned char, std::set<channelID>> iTC, std::map<channelID, channelInfo> const &channelInformation,
               std::map<channelID, std::set<std::string>> const &relevantChannels, std::set<channelID> const &doublyRelevantChannels,
               std::map<std::string, std::shared_ptr<block>> const &blocks, std::promise<findResult> promise)
{
    std::map<channelID, unsigned char> channelToIndex = std::move(cTI);
    std::map<unsigned char, std::set<channelID>> indexToChannels = std::move(iTC);
    channelID chosenChannel = constants::uninitializedChannel;
    unsigned char indexOfChosenChannel;

    bool chosen{};
    unsigned char currentIndex = constants::indexZero;
    std::set<channelID> fullyProcessedChannels{};
    bool relevantChannelFound{};
    channelID firstRelChan;
    unsigned char indexOfFirstRelChan{};

    assert(indexToChannels.contains(constants::indexZero));

    auto iTCEntry = indexToChannels.find(currentIndex);
    while (!chosen && iTCEntry != indexToChannels.end() && (!relevantChannelFound || (!doublyRelevantChannels.empty() && currentIndex < indexOfFirstRelChan + constants::additionalIterationsForDoublyRelevantChannels)))
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
                    chosen = processChannelWithIndex(neighbour, it->second, currentIndex + 1, trackToUse, channelToIndex, indexToChannels, indexOfChosenChannel, relevantChannels, doublyRelevantChannels,
                                                     relevantChannelFound, firstRelChan, indexOfFirstRelChan, blocks, channelInformation);
                    if (chosen)
                    {
                        chosenChannel = neighbour;
                        break;
                    }
                }
                else if (isChannelTrackFree(neighbour, trackToUse, channelInformation))
                {
                    chosen = processChannelWithoutIndex(neighbour, currentIndex + 1, trackToUse, channelToIndex, indexToChannels, indexOfChosenChannel, relevantChannels, doublyRelevantChannels,
                                                             relevantChannelFound, firstRelChan, indexOfFirstRelChan, blocks);
                    if (chosen)
                    {
                        chosenChannel = neighbour;
                        break;
                    }
                }
            }
            if (chosen)
                break;

            fullyProcessedChannels.emplace(channel);
        }
        if (chosen)
            break;

        currentIndex++;
        iTCEntry = indexToChannels.find(currentIndex);
    }

    if (!chosen && relevantChannelFound)
    {
        assert(relevantChannels.find(firstRelChan)->second.size() == 1);
        indexOfChosenChannel = indexOfFirstRelChan;
        chosenChannel = std::move(firstRelChan);
    }

    promise.set_value(findResult{trackToUse, std::move(channelToIndex), std::move(indexToChannels), std::move(chosenChannel), std::move(indexOfChosenChannel)});
}

findResult findPinWithThreads(std::set<unsigned char> tracksToCheck, std::map<unsigned char, std::map<channelID, unsigned char>> &channelToIndexMaps, std::map<unsigned char, std::map<unsigned char, std::set<channelID>>> &indexToChannelsMaps, unsigned char arraySize,
                             unsigned char channelWidth, std::map<channelID, channelInfo> const &channelInformation, std::map<channelID, std::set<std::string>> const &relevantChannels,
                             std::set<channelID> const &doublyRelevantChannels, std::map<std::string, std::shared_ptr<block>> const &blocks)
{
    findResult bestResult;
    unsigned char bestIndexOfChannelWithPin = std::numeric_limits<unsigned char>::max();

    std::thread threads[tracksToCheck.size()];
    std::future<findResult> futures[tracksToCheck.size()];

    unsigned char threadCount = 0;
    for (unsigned char track : tracksToCheck)
    {
        std::map<channelID, unsigned char> channelToIndex = channelToIndexMaps.find(track)->second;
        std::map<unsigned char, std::set<channelID>> indexToChannels = indexToChannelsMaps.find(track)->second;
        
        std::promise<findResult> promise;
        std::future<findResult> future = promise.get_future();

        threads[threadCount] = std::thread(findPin, track, channelWidth, arraySize, std::move(channelToIndex), std::move(indexToChannels), std::cref(channelInformation), std::cref(relevantChannels), std::cref(doublyRelevantChannels), std::cref(blocks), std::move(promise));
        futures[threadCount] = std::move(future);
        threadCount++;
    }

    threadCount = 0;
    for (unsigned char track : tracksToCheck)
    {
        findResult result = futures[threadCount].get();

        channelToIndexMaps.insert_or_assign(track, result.channelToIndex);
        indexToChannelsMaps.insert_or_assign(track, result.indexToChannels);

        if (result.chosenChannel.isInitialized() && result.indexOfChosenChannel < bestIndexOfChannelWithPin)
        {
            bestIndexOfChannelWithPin = result.indexOfChosenChannel;
            bestResult = std::move(result);
        }

        threads[threadCount].join();
        threadCount++;
    }

    return std::move(bestResult);
}