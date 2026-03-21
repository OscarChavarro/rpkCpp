#include <stdio.h>

#include "PPMWriter.h"

int PPMWriter::write(const char* filename, ImagePPM& img) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        return 0;
    }

    fprintf(f, "P6\n%d %d\n255\n", img.width, img.height);

    size_t written = fwrite(img.data, sizeof(PixelRGB), img.width * img.height, f);
    fclose(f);

    if (written != (size_t)(img.width * img.height)) {
        return 0;
    }

    return 1;
}
