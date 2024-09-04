#include "concurrentRouting.hpp"
#include "routing.hpp"
#include "logging.hpp"

#include <cassert>
#include <thread>
#include <future>

void findPin(unsigned char trackToUse, unsigned char channelWidth, unsigned char arraySize, std::map<channelID, unsigned char> &channelToIndex,
             std::map<unsigned char, std::set<channelID>> &indexToChannels, std::map<channelID, channelInfo> const &channelInformation,
             std::map<channelID, std::set<std::string>> const &relevantChannels, std::set<channelID> const &doublyRelevantChannels,
             std::map<std::string, std::shared_ptr<block>> const &blocks, std::promise<findResult> promise)
{
    channelID chosenChannel = constants::uninitializedChannel;
    unsigned char indexOfChosenChannel{};

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

            for (channelID neighbour : channel.generateNeighbours(arraySize))
            {
                if (auto it = channelToIndex.find(neighbour); it != channelToIndex.end())
                {
                    assert(it->second >= currentIndex - 1);
                    chosen = handleChannelWithIndex(neighbour, it->second, currentIndex + 1, trackToUse, channelToIndex, indexToChannels, indexOfChosenChannel, relevantChannels, doublyRelevantChannels,
                                                    relevantChannelFound, firstRelChan, indexOfFirstRelChan, blocks, channelInformation);
                    if (chosen)
                    {
                        chosenChannel = neighbour;
                        break;
                    }
                }
                else if (isChannelTrackFree(neighbour, trackToUse, channelInformation))
                {
                    chosen = handleChannelWithoutIndex(neighbour, currentIndex + 1, trackToUse, channelToIndex, indexToChannels, indexOfChosenChannel, relevantChannels, doublyRelevantChannels,
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
        chosenChannel = firstRelChan;
    }

    promise.set_value(findResult{trackToUse, chosenChannel, indexOfChosenChannel});
}

findResult findPinWithThreads(std::set<unsigned char> const &tracksToCheck, std::map<unsigned char, std::map<channelID, unsigned char>> &channelToIndexMaps, std::map<unsigned char, std::map<unsigned char, std::set<channelID>>> &indexToChannelsMaps, unsigned char arraySize,
                              unsigned char channelWidth, std::map<channelID, channelInfo> const &channelInformation, std::map<channelID, std::set<std::string>> const &relevantChannels,
                              std::set<channelID> const &doublyRelevantChannels, std::map<std::string, std::shared_ptr<block>> const &blocks)
{
    std::vector<std::thread> threads;
    std::vector<std::future<findResult>> futures;

    std::map<channelID, unsigned char> channelToIndex0 = channelToIndexMaps.find(0)->second;

    for (unsigned char track : tracksToCheck)
    {
        std::map<channelID, unsigned char> &channelToIndex = channelToIndexMaps.find(track)->second;
        std::map<unsigned char, std::set<channelID>> &indexToChannels = indexToChannelsMaps.find(track)->second;

        std::promise<findResult> promise;
        std::future<findResult> future = promise.get_future();

        threads.push_back(std::thread(findPin, track, channelWidth, arraySize, std::ref(channelToIndex), std::ref(indexToChannels), std::cref(channelInformation), std::cref(relevantChannels), std::cref(doublyRelevantChannels), std::cref(blocks), std::move(promise)));
        futures.push_back(std::move(future));
    }

    findResult bestResult;
    unsigned char bestIndexOfChannelWithPin = std::numeric_limits<unsigned char>::max();

    for (int threadCount = 0; threadCount < threads.size(); threadCount++)
    {
        findResult result = futures[threadCount].get();

        if (result.chosenChannel.isInitialized() && result.indexOfChosenChannel < bestIndexOfChannelWithPin)
        {
            bestResult = result;
            bestIndexOfChannelWithPin = bestResult.indexOfChosenChannel;
        }

        threads[threadCount].join();
    }

    return bestResult;
}