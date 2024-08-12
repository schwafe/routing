#include <iostream>
#include <cassert>
#include "logging.hpp"

std::string channelIDToString(channelID channel)
{
    return "(" + std::to_string(+channel.getXCoordinate()) + ',' + std::to_string(+channel.getYCoordinate()) + ',' + channel.getType() + ')';
}

void printIndices(std::map<channelID, unsigned char> indices)
{
    std::cout << "Indices (x,y,type) - index:" << std::endl;
    for (auto &entry : indices)
        std::cout << channelIDToString(entry.first) << " - " << +entry.second << std::endl;
    std::cout << std::endl;
}

void printConnections(net net)
{
    std::cout << "net sourceBlockName: '" << net.getSourceBlockName() << "' size: " << net.getPinCount() << std::endl;
    for (auto &entry : net.getConnectedPinBlockNamesAndTheirRouting())
    {
        std::stack<std::pair<channelID, unsigned char>> connection{entry.second};
        assert(!connection.empty());

        std::cout << "\tsinkBlockName: '" << entry.first << "'" << std::endl;
        do
        {
            std::cout << "\t\t" << channelIDToString(connection.top().first) << " - " << connection.top().second << std::endl;
            connection.pop();
        } while (!connection.empty());
    }
}