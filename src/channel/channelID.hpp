#ifndef CHANNELID_H
#define CHANNELID_H

#include <set>

class channelID
{
    unsigned char x{};
    unsigned char y{};
    char type{}; // x or y - horizontal or vertical - channel

public:
    auto operator<=>(channelID const &rhs) const = default;
    channelID(unsigned char x, unsigned char y, char type) : x(x), y(y), type(type) {};
    channelID() {};

    bool isInitialized() const;
    unsigned char getXCoordinate() const;
    unsigned char getYCoordinate() const;
    char getType() const;
    std::set<channelID> getNeighbours(unsigned char arraySize) const;
};

#endif