#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "NativeRandom.h"

void
nativeRandomStartup() {
    printf("WARNING: Using native C random native generator\n");
    fflush(stdout);
}

void
nativeRandomShutdown() {
}

void
nativeSrand48(long seed) {
    srand48(seed);
}

double
nativeDrand48() {
    return drand48();
}

void
nativeSeed48(const unsigned short newSeed[3], unsigned short outOldSeed[3]) {
    unsigned short *previous = seed48(const_cast<unsigned short *>(newSeed));
    std::memcpy(outOldSeed, previous, 3 * sizeof(unsigned short));
}
