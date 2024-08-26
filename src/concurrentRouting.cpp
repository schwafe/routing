#include "concurrentRouting.hpp"

#include <cassert>
#include <thread>
#include <atomic>
#include <shared_mutex>

std::shared_mutex cTIMutex;
std::shared_mutex iTCMutex;
std::shared_mutex fullyProcessedMutex;

namespace constants
{
    const unsigned int maximumNumberOfThreads = std::thread::hardware_concurrency() != 0 ? std::thread::hardware_concurrency() : 4;
    const unsigned char channelWorkloadPerThread = 8;
}

void registerIndexConcurrently(channelID channel, unsigned char index, std::map<channelID, unsigned char> &channelToIndex, std::map<unsigned char, std::set<channelID>> &indexToChannels)
{
    {
        const std::lock_guard<std::shared_mutex> cTILock(cTIMutex);
        channelToIndex.insert_or_assign(channel, index);
    }
    {
        const std::lock_guard<std::shared_mutex> iTCLock(iTCMutex);
        auto result = indexToChannels.find(index);
        if (result == indexToChannels.end())
            result = indexToChannels.emplace(index, std::set<channelID>{}).first;
        result->second.emplace(channel);
    }
}

bool testForRelevantChannelAndResult(channelID channel, unsigned char indexOfChannel, std::atomic_flag &gotResult, channelID &result, bool doublyRelChanExist, std::map<channelID, std::set<std::string>> const &relevantChannels, unsigned char &indexOfResult, std::atomic_flag &relevantChannelFound, channelID &firstRelChan, unsigned char &indexOfFirstRelChan, std::shared_ptr<std::atomic_flag> const &done, std::atomic_flag &anyThreadDone)
{
    if (!relevantChannelFound.test() && relevantChannels.contains(channel))
    {
        if (!relevantChannelFound.test_and_set())
        {
            if (doublyRelChanExist)
            {
                firstRelChan = channel;
                indexOfFirstRelChan = indexOfChannel;
            }
            else
            {
                bool firstResult = !gotResult.test_and_set();
                assert(firstResult);
                result = channel;
                indexOfResult = indexOfChannel;

                done->test_and_set();
                anyThreadDone.test_and_set();
                anyThreadDone.notify_one();
                return true;
            }
        }
    }
    return false;
}

void processChannelFully(std::stop_token stopToken, std::set<channelID> channels, unsigned char currentIndex, unsigned char arraySize, unsigned char channelWidth, bool doublyRelChanExist, std::map<channelID, channelInfo> const &channelInformation,
                         std::map<channelID, std::set<std::string>> const &relevantChannels, std::set<channelID> const &doublyRelevantChannels, std::map<channelID, unsigned char> &channelToIndex,
                         std::map<unsigned char, std::set<channelID>> &indexToChannels, std::set<channelID> &fullyProcessedChannels, std::atomic_flag &gotResult, channelID &result,
                         unsigned char &indexOfResult, std::atomic_flag &relevantChannelFound, channelID &firstRelChan, unsigned char &indexOfFirstRelChan, std::shared_ptr<std::atomic_flag> const &done, std::atomic_flag &anyThreadDone)
{
    for (channelID channel : channels)
    {
        for (channelID neighbour : channel.getNeighbours(arraySize))
        {
            if (stopToken.stop_requested())
                return;

            bool channelHasIndex{};
            unsigned char indexOfNeighbour{};
            {
                std::shared_lock<std::shared_mutex> cTILock(cTIMutex);
                auto it = channelToIndex.find(neighbour);
                channelHasIndex = it != channelToIndex.end();
                if (channelHasIndex)
                    indexOfNeighbour = it->second;
            }

            if (channelHasIndex)
            {
                if (indexOfNeighbour > currentIndex + 1)
                {
                    indexOfNeighbour = currentIndex + 1;
                    registerIndexConcurrently(neighbour, indexOfNeighbour, channelToIndex, indexToChannels);
                }

                bool resultFound = testForRelevantChannelAndResult(neighbour, indexOfNeighbour, gotResult, result, doublyRelChanExist, relevantChannels, indexOfResult, relevantChannelFound, firstRelChan, indexOfFirstRelChan, done, anyThreadDone);
                if (resultFound)
                    return;
            }
            else if (!isChannelFull(neighbour, channelInformation, channelWidth))
            {
                unsigned char indexOfNeighbour = currentIndex + 1;
                registerIndexConcurrently(neighbour, indexOfNeighbour, channelToIndex, indexToChannels);

                if (doublyRelChanExist && doublyRelevantChannels.contains(neighbour))
                {
                    bool firstResult = !gotResult.test_and_set();
                    if (firstResult)
                    {
                        result = neighbour;
                        indexOfResult = indexOfNeighbour;
                    }
                    done->test_and_set();
                    anyThreadDone.test_and_set();
                    anyThreadDone.notify_one();
                    return;
                }
                else
                {
                    bool resultFound = testForRelevantChannelAndResult(neighbour, indexOfNeighbour, gotResult, result, doublyRelChanExist, relevantChannels, indexOfResult, relevantChannelFound, firstRelChan, indexOfFirstRelChan, done, anyThreadDone);
                    if (resultFound)
                        return;
                }
            }
        }

        {
            const std::lock_guard<std::shared_mutex> fullyProcessedLock(fullyProcessedMutex);
            fullyProcessedChannels.emplace(channel);
        }
    }

    done->test_and_set();
    anyThreadDone.test_and_set();
    anyThreadDone.notify_one();
    return;
}

void chooseChannelWithPinCC(channelID const &chosenChannel, unsigned char indexOfChosenChannel, std::map<channelID, std::set<std::string>> &relevantChannels,
                          std::set<channelID> &doublyRelevantChannels, std::set<std::string> &reachedBlocks, std::map<std::string, std::shared_ptr<block>> const &blocks)
{
    for (std::string associatedBlockName : relevantChannels.find(chosenChannel)->second)
    {
        reachedBlocks.insert(associatedBlockName);
        std::shared_ptr<block> p_block = blocks.find(associatedBlockName)->second;
        p_block->setChannelTaken(chosenChannel);

        for (channelID channel : p_block->getOpenChannels())
        {
            assert(relevantChannels.contains(channel));
            std::set<std::string> &associatedBlocks = relevantChannels.find(channel)->second;
            associatedBlocks.erase(associatedBlockName);
            if (!associatedBlocks.empty())
            {
                assert(doublyRelevantChannels.contains(channel));
                doublyRelevantChannels.erase(channel);
            }
            else
                relevantChannels.erase(channel);
        }
    }

    relevantChannels.erase(chosenChannel);
    doublyRelevantChannels.erase(chosenChannel);
}

channelID findPindConcurrently(std::map<channelID, unsigned char> &channelToIndex, std::map<unsigned char, std::set<channelID>> &indexToChannels, unsigned char arraySize, unsigned char &indexOfChosenChannel,
                               unsigned char channelWidth, std::map<channelID, channelInfo> const &channelInformation, std::map<channelID, std::set<std::string>> &relevantChannels, std::set<channelID> &doublyRelevantChannels,
                               std::set<std::string> &reachedBlocks, std::map<std::string, std::shared_ptr<block>> const &blocks)
{
    unsigned char currentIndex = constants::indexZero;
    std::set<channelID> fullyProcessedChannels{};
    std::vector<std::jthread> threads{};
    std::vector<std::shared_ptr<std::atomic_flag>> doneFlags{};
    std::atomic_flag gotResult{};
    std::atomic_flag anyThreadDone{};
    channelID result{};
    unsigned char indexOfResult = std::numeric_limits<unsigned char>::max();
    std::atomic_flag relevantChannelFound{};
    channelID firstRelChan{};
    unsigned char indexOfFirstRelChan{};
    const bool doublyRelChanExist = doublyRelevantChannels.empty();

    threads.reserve(constants::maximumNumberOfThreads);
    doneFlags.reserve(constants::maximumNumberOfThreads);

    assert(constants::maximumNumberOfThreads >= 1);

    for (int index = 0; index < constants::maximumNumberOfThreads; index++)
    {
        threads.push_back(std::jthread{});
        doneFlags.push_back(std::make_shared<std::atomic_flag>());
        doneFlags[index]->test_and_set();
    }

    assert(indexToChannels.contains(constants::indexZero));

    while (!gotResult.test() && indexToChannels.contains(currentIndex))
    {
        std::set<channelID> channelsOfCurrentIndex = indexToChannels.find(currentIndex)->second;

        anyThreadDone.clear();

        auto it = channelsOfCurrentIndex.begin();
        while (it != channelsOfCurrentIndex.end())
        {
            for (int index = 0; index < constants::maximumNumberOfThreads; index++)
            {
                if (doneFlags[index])
                {
                    doneFlags[index]->clear();

                    std::set<channelID> channels;
                    {
                        std::shared_lock<std::shared_mutex> fullyProcessedLock(fullyProcessedMutex);
                        for (unsigned char i = 0; i < constants::channelWorkloadPerThread; i++)
                        {
                            while (it != channelsOfCurrentIndex.end() && fullyProcessedChannels.contains(*it))
                                it++;

                            if (it != channelsOfCurrentIndex.end())
                            {
                                channels.insert(*it);
                                it++;
                            }
                            else
                                break;
                        }
                    }
                    if (channels.empty())
                        break;

                    if (threads[index].joinable())
                    {
                        threads[index].join();
                    }

                    threads[index] = std::jthread(processChannelFully, channels, currentIndex, arraySize, channelWidth, doublyRelChanExist, std::cref(channelInformation), std::cref(relevantChannels),
                                                  std::cref(doublyRelevantChannels), std::ref(channelToIndex), std::ref(indexToChannels), std::ref(fullyProcessedChannels), std::ref(gotResult),
                                                  std::ref(result), std::ref(indexOfResult), std::ref(relevantChannelFound), std::ref(firstRelChan), std::ref(indexOfFirstRelChan),
                                                  std::ref(doneFlags[index]), std::ref(anyThreadDone));
                }
            }

            anyThreadDone.wait(false);
            anyThreadDone.clear();
            if (gotResult.test())
                break;
        }

        assert(indexToChannels.find(currentIndex)->second.size() == channelsOfCurrentIndex.size());

        if (gotResult.test())
        {
            for (std::jthread &thread : threads)
                thread.request_stop();
        }
        else if (!gotResult.test() && relevantChannelFound.test() && currentIndex + 1 == indexOfFirstRelChan + constants::additionalIterationsForDoublyRelevantChannels)
        {
            gotResult.test_and_set();
            result = firstRelChan;
            indexOfResult = indexOfFirstRelChan;
        }
        else
            currentIndex++;

        int counter{};
        for (std::jthread &thread : threads)
            if (thread.joinable())
            {
                counter++;
                thread.join();
            }
    }

    if (gotResult.test())
    {
        chooseChannelWithPinCC(result, indexOfResult, relevantChannels, doublyRelevantChannels, reachedBlocks, blocks);
        indexOfChosenChannel = indexOfResult;
        return result;
    }
    else
        return constants::uninitializedChannel;
}