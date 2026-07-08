#ifndef NATIVE_RANDOM_NUMBER_GENERATOR_H
#define NATIVE_RANDOM_NUMBER_GENERATOR_H

extern "C" {

void nativeRandomStartup();
void nativeRandomShutdown();

void nativeSrand48(long seed);
double nativeDrand48();

void nativeSeed48(const unsigned short newSeed[3], unsigned short outOldSeed[3]);

}

#endif
