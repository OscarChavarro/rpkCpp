#ifndef __RAY_TRACER_STATISTICS__
#define __RAY_TRACER_STATISTICS__

class RayTracer;

class RayTracerStatistics {
  public:
    RayTracer *currentRayTracer;
    double totalTime;
    long rayCount;
    long pixelCount;

    RayTracerStatistics();
    void resetCounters();
};

#endif
