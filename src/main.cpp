#include <iostream>
#include <fstream>
#include <regex>
#include "constants.hpp"
#include "channel.hpp"
#include "routing.hpp"
#include "block.hpp"

void failIfFalse(bool condition, const unsigned char errorCode, std::string failMessage)
{
    if (!condition)
    {
        std::cerr << failMessage << std::endl;
        exit(errorCode);
    }
}

void failIfInvalidRead(bool valid, std::string failMessage)
{
    failIfFalse(valid, constants::readFailure, failMessage);
}

void readNet(std::string fileName, std::map<std::string, std::shared_ptr<net>> &netsByNameOfTheSourceBlock, std::map<std::string, block> &blocks)
{
    std::string line;
    std::ifstream netFile;
    netFile.open(constants::netPrefix + fileName + constants::netSuffix);

    std::map<std::string, std::shared_ptr<net>> netsByNameOfTheNet;

    while (std::getline(netFile, line))
    {
        std::smatch matches;
        std::string blockName;
        if (std::regex_match(line, matches, constants::clbPattern))
        {
            blockName = matches[1];

            std::getline(netFile, line);
            std::regex_match(line, matches, constants::clbPinPattern);
            // TODO  matches[5] is the clock !!
            for (unsigned char index = 0; index < 3; index++)
            {
                if (matches[index] != "open")
                {
                    if (netsByNameOfTheNet.contains(matches[index]))
                    {
                        netsByNameOfTheNet.find(matches[index])->second->setConnectedPin(blockName);
                    }
                    else
                    {
                        std::shared_ptr<net> p_net = std::make_shared<net>();
                        p_net->setConnectedPin(blockName);
                        netsByNameOfTheNet.emplace(matches[index], p_net);
                    }
                }
            }

            if (!netsByNameOfTheNet.contains(matches[4]))
            {
                netsByNameOfTheNet.emplace(matches[4], std::make_shared<net>());
            }
            netsByNameOfTheSourceBlock.emplace(blockName, std::shared_ptr<net>(netsByNameOfTheNet.find(matches[4])->second));

            // for a simple architecture (at most one 4-LUT + one FF per clb) subblock line does not provide additional information
            std::getline(netFile, line);
            failIfInvalidRead(std::regex_match(line, matches, constants::clbSubPattern), "No subblock line after the pinlist of clb " + blockName);

            std::getline(netFile, line);
            failIfInvalidRead(line.empty(), "No empty line after the subblock line of clb " + blockName);
        }
        else if (std::regex_match(line, matches, constants::outputPattern))
        {
            blockName = matches[1];

            std::getline(netFile, line);
            std::regex_match(line, matches, constants::padPinPattern);

            if (netsByNameOfTheNet.contains(matches[1]))
            {
                netsByNameOfTheNet.find(matches[1])->second->setConnectedPin(blockName);
            }
            else
            {
                std::shared_ptr<net> p_net = std::make_shared<net>();
                p_net->setConnectedPin(blockName);
                netsByNameOfTheNet.emplace(matches[1], p_net);
            }

            std::getline(netFile, line);
            failIfInvalidRead(line.empty(), "No empty line after the pinlist line of output " + blockName);
        }
        else if (std::regex_match(line, matches, constants::inputPattern))
        {
            blockName = matches[1];

            std::getline(netFile, line);
            std::regex_match(line, matches, constants::padPinPattern);

            if (!netsByNameOfTheNet.contains(matches[1]))
            {
                netsByNameOfTheNet.emplace(matches[1], std::make_shared<net>());
            }
            netsByNameOfTheSourceBlock.emplace(blockName, std::shared_ptr<net>(netsByNameOfTheNet.find(matches[1])->second));

            std::getline(netFile, line);
            failIfInvalidRead(line.empty(), "No empty line after the pinlist line of input " + blockName);
        }
        else if (std::regex_match(line, matches, constants::globalPattern))
        {
            blockName = matches[1];

            std::getline(netFile, line);
            failIfInvalidRead(line.empty(), "No empty line after global declaration of " + blockName);
        }
        else
        {
            // expecting file to end
            failIfInvalidRead(line.empty(), "Invalid line: '" + line + "'.");

            std::string secLine;
            failIfInvalidRead(!std::getline(netFile, secLine) && netFile.eofbit, "Expected file to end after two empty lines, but instead got this line: '" + secLine + "'.");
            std::cout << "Netfile was successfully read!" << std::endl;
        }
    }
    netFile.close();
}

void readPlace(std::string fileName, unsigned char &arraySize, std::map<std::string, block> &blocks)
{
    std::string line;
    std::ifstream placementFile;
    placementFile.open(constants::placementPrefix + fileName + constants::placementSuffix);

    // netlist and architecture file line
    std::getline(placementFile, line);

    // array size line
    std::smatch matches;
    std::getline(placementFile, line);
    failIfInvalidRead(std::regex_search(line, matches, constants::arraySizePattern), "No array size given or format unknown!");
    arraySize = std::stoi(matches[1]);

    // empty line
    std::getline(placementFile, line);

    bool matched;
    int i = 0;
    do
    {
        std::getline(placementFile, line);

        while (std::regex_match(line, constants::commentPattern))
        {
            // skip comment lines
            std::getline(placementFile, line);
        }

        std::smatch matches;
        matched = std::regex_search(line, matches, constants::inputPattern);
        if (matched)
        {
            std::cout << "match!" << std::endl;

            for (auto match : matches)
            {
                std::cout << match << std::endl;
            }
        }

    } while (matched && i++ <= 10);

    placementFile.close();
}

int main(int argc, char *argv[])
{
    failIfFalse(argc == 2, constants::wrongArguments, "Argument count unexpected! Please enter just one argument: the filename for the .net and .place files to be used.");
    std::string fileName = argv[1];

    unsigned char arraySize;
    std::map<std::string, block> blocks;
    std::map<std::string, std::shared_ptr<net>> netsByNameOfTheSourceBlock;
    readNet(fileName, netsByNameOfTheSourceBlock, blocks);
    readPlace(fileName, arraySize, blocks);

    unsigned char maxTracks = constants::startingValueMaxTracks;

    routeNets(arraySize, maxTracks, netsByNameOfTheSourceBlock, blocks);
    // TODO try lower maxTracks values

    return constants::success;
}