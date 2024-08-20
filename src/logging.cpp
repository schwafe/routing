#include <iostream>
#include <cassert>
#include "logging.hpp"

std::string channelIDToString(channelID const &channel)
{
    return "(" + std::to_string(+channel.getXCoordinate()) + ',' + std::to_string(+channel.getYCoordinate()) + ',' + channel.getType() + ')';
}

std::string argsToString(int argc, char *argv[])
{
    std::string args = "";
    for (int index = 0; index < argc; index++)
    {
        args += argv[index];
        args += "' '";
    }
    return args.substr(0, args.size() - 2);
}

std::string listConnectedBlocks(std::shared_ptr<net> const &p_net)
{
    std::string list{};
    for (std::string sinkBlockName : p_net->getSinkBlockNames())
        list.append(sinkBlockName).append(", ");
    return list.substr(0, list.size() - 2);
}

void printIndices(std::map<channelID, unsigned char> const &indices)
{
    std::cout << '\n'
              << "Indices (x,y,type) - index:" << '\n';
    for (auto &entry : indices)
        std::cout << channelIDToString(entry.first) << " - " << +entry.second << '\n';
    std::cout << '\n';
    std::cout << std::endl;
}

void printConnections(std::shared_ptr<net> const &p_net)
{
    std::cout << "net sourceBlockName: '" << p_net->getSourceBlockName() << "' size: " << p_net->getSinkBlockCount() << '\n';
    for (auto &entry : p_net->getConnectionsByRoutingOrder())
    {
        std::vector<std::pair<channelID, unsigned char>> connection{entry.second};
        assert(!connection.empty());

        std::cout << "\tsinkBlockName: '" << entry.first << "'" << '\n';
        for (std::pair<channelID, unsigned char> connectionPair : connection)
            std::cout << "\t\t" << channelIDToString(connectionPair.first) << " - " << +connectionPair.second << '\n';
    }
    std::cout << std::endl;
}

void printLogMessage(std::string const &loggingMessage) {
    std::cout << loggingMessage << std::endl;
}