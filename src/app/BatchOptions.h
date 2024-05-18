#ifndef RPK_BATCHOPTIONS_H
#define RPK_BATCHOPTIONS_H

class BatchOptions {
  public:
    int iterations; // Radiance method iterations
    const char *radianceImageFileNameFormat;
    const char *radianceModelFileNameFormat;
    int saveModulo; // Every n-th iteration, surface model and image will be saved
    const char *raytracingImageFileName;
    int timings = false;

    BatchOptions();
    virtual ~BatchOptions();
};

#endif
