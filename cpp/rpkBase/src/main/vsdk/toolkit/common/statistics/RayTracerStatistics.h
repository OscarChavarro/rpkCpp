#ifndef RAY_TRACER_STATISTICS__
#define RAY_TRACER_STATISTICS__

class RayTracerStatistics {
  public:
    double totalTime;
    long rayCount;
    long pixelCount;

    RayTracerStatistics();
    void resetCounters();
};

#endif
