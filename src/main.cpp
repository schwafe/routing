#include <iostream>
#include <fstream>
#include <regex>
#include "constants.hpp"

void failIfFalse(bool condition, const unsigned char errorCode, std::string failMessage)
{
    if (!condition)
    {
        std::cerr << failMessage << std::endl;
        exit(errorCode);
    }
}

void failIfInvalidRead(bool valid, std::string failMessage) {
    failIfFalse(valid, constants::readFailure, failMessage);
}

void readNet(std::string fileName)
{
    std::string line;
    std::ifstream netFile;
    netFile.open(constants::netPrefix + fileName + constants::netSuffix);

    while(std::getline(netFile, line))
    {
        std::smatch matches;
        std::string name;
        if (std::regex_match(line, matches, constants::clbPattern))
        {
            name = matches[1];

            std::getline(netFile, line);
            std::regex_match(line, matches, constants::clbPinPattern);
            // TODO process - matches[1-3] are inputs, matches[4] is the output and matches[5] is the clock

            // for a simple architecture (at most one 4-LUT + one FF per clb) subblock line does not provide additional information
            std::getline(netFile, line);
            failIfInvalidRead(std::regex_match(line, matches, constants::clbSubPattern), "No subblock line after the pinlist of clb " + name);

            std::getline(netFile, line);
            failIfInvalidRead(line.empty(), "No empty line after the subblock line of clb " + name);
        } else if(std::regex_match(line, matches, constants::outputPattern)) {
            name = matches[1];

            std::getline(netFile, line);
            std::regex_match(line, matches, constants::padPinPattern);
            // TODO process - matches[1] is the output

            std::getline(netFile, line);
            failIfInvalidRead(line.empty(), "No empty line after the pinlist line of output " + name);
        } else if(std::regex_match(line, matches, constants::inputPattern)) {
            name = matches[1];

            std::getline(netFile, line);
            std::regex_match(line, matches, constants::padPinPattern);
            // TODO process - matches[1] is the input
            
            std::getline(netFile, line);
            failIfInvalidRead(line.empty(), "No empty line after the pinlist line of input " + name);
        } else if(std::regex_match(line, matches, constants::globalPattern)) {
            name = matches[1];
            
            std::getline(netFile, line);
            failIfInvalidRead(line.empty(), "No empty line after global declaration of " + name);
        } else {
            //expecting file to end
            failIfInvalidRead(line.empty(), "Invalid line: '" + line + "'.");

            std::string secLine;
            failIfInvalidRead(!std::getline(netFile, secLine) && netFile.eofbit, "Expected file to end after two empty lines, but instead got this line: '" + secLine + "'.");
            std::cout << "Netfile was successfully read!" << std::endl;
        }
    }
    netFile.close();
}

void readPlace(std::string fileName)
{
    std::string line;
    std::ifstream placementFile;
    placementFile.open(constants::placementPrefix + fileName + constants::placementSuffix);

    // netlist and architecture file line
    std::getline(placementFile, line);
    // array size line
    std::getline(placementFile, line);
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

    readNet(fileName);
    return constants::success;
}