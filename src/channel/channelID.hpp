#ifndef CHANNELID_H
#define CHANNELID_H

#include <set>
#include <cstdint>

enum channelType : std::int8_t
{
    uninitialised,
    horizontal,
    vertical
};

class channelID
{
    unsigned char x{};
    unsigned char y{};
    channelType type = channelType::uninitialised;

public:
    auto operator<=>(channelID const &rhs) const = default;
    channelID(unsigned char x, unsigned char y, channelType type) : x(x), y(y), type(type) {};
    channelID() {};

    unsigned char getXCoordinate() const;
    unsigned char getYCoordinate() const;
    channelType getType() const;
    bool isInitialised() const;

    std::set<channelID> generateNeighbours(unsigned char arraySize) const;
};

#endif