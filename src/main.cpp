#include <iostream>
#include <fstream>
#include <regex>
#include "constants.hpp"

void failIfFailed(std::ifstream &file)
{
    if (file.fail())
    {
        std::cout << "The line was too long!";
        file.close();
        exit(constants::readFailure);
    }
}

int main()
{

    std::ifstream placementFile;
    placementFile.open(constants::placementPrefix + "alu4.place");

    char cLine[constants::maxLineLength];

    // netlist and architecture file line
    placementFile.getline(cLine, constants::maxLineLength);
    std::cout << cLine << std::endl;
    failIfFailed(placementFile);

    // array size line
    placementFile.getline(cLine, constants::maxLineLength);
    std::cout << cLine << std::endl;
    failIfFailed(placementFile);

    if (placementFile.peek() == '\n')
    {
        // skip empty line
        placementFile.getline(cLine, constants::maxLineLength);
        failIfFailed(placementFile);
    }

    bool matched;
    int i = 0;
    do
    {
        placementFile.getline(cLine, constants::maxLineLength);
        failIfFailed(placementFile);
        std::string line(cLine);

        while (std::regex_match(line, constants::commentPattern))
        {
            // skip comment lines
            placementFile.getline(cLine, constants::maxLineLength);
            failIfFailed(placementFile);
            line = cLine;
        }

        // TODO check for EOF

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
    return constants::success;
}