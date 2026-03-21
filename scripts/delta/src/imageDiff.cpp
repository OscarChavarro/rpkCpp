#include <stdio.h>
#include <stdlib.h>

#include "ImageComparator.h"
#include "ImagePPM.h"
#include "PPMReader.h"
#include "PPMWriter.h"

int main(int argc, char** argv) {
    if (argc != 4 && argc != 5) {
        fprintf(stderr, "Usage: %s A.ppm B.ppm C.ppm [threshold]\n", argv[0]);
        return 1;
    }

    const char* fileA = argv[1];
    const char* fileB = argv[2];
    const char* fileC = argv[3];

    FILE* testA = fopen(fileA, "rb");
    if (!testA) {
        fprintf(stderr, "Cannot open input file A\n");
        return 1;
    }
    fclose(testA);

    FILE* testB = fopen(fileB, "rb");
    if (!testB) {
        fprintf(stderr, "Cannot open input file B\n");
        return 1;
    }
    fclose(testB);

    ImagePPM imgA;
    ImagePPM imgB;
    ImagePPM imgOut;

    if (!PPMReader::read(fileA, imgA)) {
        fprintf(stderr, "Error reading file A\n");
        return 1;
    }

    if (!PPMReader::read(fileB, imgB)) {
        fprintf(stderr, "Error reading file B\n");
        return 1;
    }

    double threshold = 1.1;
    if (argc == 5) {
        char* endPtr = NULL;
        threshold = strtod(argv[4], &endPtr);
        if (endPtr == argv[4] || *endPtr != '\0') {
            fprintf(stderr, "Invalid threshold value. Expected a number.\n");
            return 1;
        }
    }

    if (!ImageComparator::compare(imgA, imgB, imgOut, threshold)) {
        fprintf(stderr, "Error comparing images\n");
        return 1;
    }

    if (!PPMWriter::write(fileC, imgOut)) {
        fprintf(stderr, "Error writing output file\n");
        return 1;
    }

    return 0;
}
