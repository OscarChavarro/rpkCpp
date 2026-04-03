#include <stdio.h>

#include "Stats.h"

Stats::Stats()
    : minValue(0),
      maxValue(0),
      hasValues(false),
      minDifference(0.0),
      maxDifference(0.0),
      hasDifferences(false) {}

void Stats::track(unsigned char value) {
    if (!hasValues) {
        minValue = value;
        maxValue = value;
        hasValues = true;
        return;
    }

    if (value < minValue) {
        minValue = value;
    }
    if (value > maxValue) {
        maxValue = value;
    }
}

void Stats::trackPixel(const PixelRGB& pixel) {
    track(pixel.r);
    track(pixel.g);
    track(pixel.b);
}

void Stats::trackDifference(double difference) {
    if (!hasDifferences) {
        minDifference = difference;
        maxDifference = difference;
        hasDifferences = true;
        return;
    }

    if (difference < minDifference) {
        minDifference = difference;
    }
    if (difference > maxDifference) {
        maxDifference = difference;
    }
}

void Stats::printReport() const {
    if (!hasValues) {
        printf("Stats report: no pixel data processed\n");
        return;
    }

    printf("Stats report: min unsigned char = %u, max unsigned char = %u\n",
           static_cast<unsigned int>(minValue),
           static_cast<unsigned int>(maxValue));

    if (!hasDifferences) {
        printf("Stats report: no pixel differences processed\n");
        return;
    }

    printf("Stats report: min pixel difference = %.6f, max pixel difference = %.6f\n",
           minDifference,
           maxDifference);
}
