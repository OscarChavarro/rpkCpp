#ifndef STATS_H
#define STATS_H

#include "PixelRGB.h"

class Stats {
public:
    Stats();

    void track(unsigned char value);
    void trackDifference(double difference);
    void trackPixel(const PixelRGB& pixel);
    void printReport() const;

private:
    unsigned char minValue;
    unsigned char maxValue;
    bool hasValues;
    double minDifference;
    double maxDifference;
    bool hasDifferences;
};

#endif
