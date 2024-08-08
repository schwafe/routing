#ifndef CHANNEL_H
#define CHANNEL_H

#include <map>
#include <set>

class channelID
{
    unsigned char x;
    unsigned char y;
    char type; // x or y - horizontal or vertical - channel

public:
    channelID(unsigned char x, unsigned char y, char type);
    unsigned char getXCoordinate();
    unsigned char getYCoordinate();
    char getType();
};

class channelInfo
{
    unsigned char usedTracks;
    unsigned short tracks;

public:
    bool isFull(unsigned char maxTracks);
};

#endif