#include <stdio.h>
#include <string.h>

#include "PPMReader.h"

int PPMReader::read(const char* filename, ImagePPM& img) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        return 0;
    }

    char magic[3];
    if (fscanf(f, "%2s", magic) != 1) {
        fclose(f);
        return 0;
    }

    if (strcmp(magic, "P6") != 0) {
        fclose(f);
        return 0;
    }

    int width, height, maxval;

    int c = fgetc(f);
    while (c == '#') {
        while (fgetc(f) != '\n')
            ;
        c = fgetc(f);
    }
    ungetc(c, f);

    if (fscanf(f, "%d %d", &width, &height) != 2) {
        fclose(f);
        return 0;
    }

    if (fscanf(f, "%d", &maxval) != 1) {
        fclose(f);
        return 0;
    }

    if (maxval != 255) {
        fclose(f);
        return 0;
    }

    fgetc(f);

    if (!img.allocate(width, height)) {
        fclose(f);
        return 0;
    }

    size_t readBytes = fread(img.data, sizeof(PixelRGB), width * height, f);
    fclose(f);

    if (readBytes != static_cast<size_t>(width * height)) {
        return 0;
    }

    return 1;
}
