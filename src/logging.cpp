#include <iostream>
#include <cassert>
#include "logging.hpp"

std::string channelIDToString(channelID const &channel)
{
    std::string msg = "(" + std::to_string(+channel.getXCoordinate()) + ',' + std::to_string(+channel.getYCoordinate()) + ',';
    if (channel.getType() == channelType::horizontal)
        return msg + "X)";
    else
        return msg + "Y)";
}

std::string argsToString(int argc, char *argv[])
{
    std::string args = "";
    // starting at 1, because the first parameter is not input by the user
    for (int index = 1; index < argc; index++)
    {
        args += argv[index];
        args += "' '";
    }
    return args.substr(0, args.size() - 2);
}

std::string listConnectedBlocks(std::shared_ptr<net> const &p_net)
{
    std::string list{};
    for (std::string const &connectedBlockName : p_net->getNamesOfConnectedBlocks())
        list.append(connectedBlockName).append(", ");
    return list.substr(0, list.size() - 2);
}

void printChannelToIndex(std::map<channelID, unsigned char> const &channelToIndex)
{
    std::cout << '\n'
              << "channelToIndex (x,y,type) - index:" << '\n';
    for (auto &entry : channelToIndex)
        std::cout << channelIDToString(entry.first) << " - " << +entry.second << '\n';
    std::cout << '\n';
    std::cout << std::endl;
}

void printConnections(std::shared_ptr<net> const &p_net)
{
    std::cout << "net sourceBlockName: '" << p_net->getSourceBlockName() << "' size: " << p_net->getConnectedBlockCount() << '\n';
    for (auto &entry : p_net->getConnectionsByRoutingOrder())
    {
        unsigned char track = entry.second.first;
        std::vector<channelID> connection = entry.second.second;
        assert(!connection.empty());

        std::cout << "\tconnectedBlockName: '" << entry.first << "'" << '\n';
        for (channelID channel : connection)
            std::cout << "\t\t" << channelIDToString(channel) << " - " << +track << '\n';
    }
    std::cout << std::endl;
}

void printLogMessage(std::string const &loggingMessage)
{
    std::cout << loggingMessage << std::endl;
}