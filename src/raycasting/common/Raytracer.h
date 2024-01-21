#ifndef _RAYTRACE_H_
#define _RAYTRACE_H_

#include "IMAGE/imagec.h"

/**
TODO SITHMASTER: This should be converted on to the Raytracer interface for inheriting the current four
raytracers: RayMatter, RayCaster, BidirectionalPathRaytracer and StochasticRaytracer.
*/

class Raytracer {
  public:
    /* short name for the raytracing method, for use as argument of
     * -raytracing command line optoin. */
    const char *shortName;

    /* how short can the short name be abbreviated? */
    int nameAbbrev;

    /* full name of the raytracing method */
    const char *fullName;

    /* function for setting default values etc... */
    void (*Defaults)();

    /* function for parsing method specific command line options */
    void (*ParseOptions)(int *argc, char **argv);

    /* Initializes the current scene for raytracing computations.
     *  Called when a new scene is loaded or when selecting a particular
     * raytracing algorithm. */
    void (*Initialize)();

    /* Raytrace the current scene as seen with the current camera. If 'ip'
     * is not a nullptr pointer, write the raytraced image using the image output
     * handle pointerd to by 'ip' . */
    void (*Raytrace)(ImageOutputHandle *ip);

    /* Redisplays last raytraced image. Returns FALSE if there is no
     * previous raytraced image and TRUE there is. */
    int (*Redisplay)();

    /* Saves last raytraced image in the file describe dby the image output
     * handle. */
    int (*SaveImage)(ImageOutputHandle *ip);

    /* Interrupts raytracing */
    void (*InterruptRayTracing)();

    /* terminate raytracing computations */
    void (*Terminate)();
};

extern double GLOBAL_raytracer_totalTime; // statistics: raytracing time
extern long GLOBAL_raytracer_rayCount; // statistics: number of rays traced
extern long GLOBAL_raytracer_pixelCount; // statistics: number of pixels drawn
extern Raytracer *GLOBAL_raytracer_activeRaytracer; // current raytracing method

extern void rayTrace(char *filename, FILE *fp, int ispipe);

/* initializes and makes current a new raytracing newMethod */
extern void setRayTracing(Raytracer *newMethod);

#endif
