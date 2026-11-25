#ifndef __BATCH_OPTIONS__
#define __BATCH_OPTIONS__

class BatchOptions {
  public:
    bool exportBinary;
    const char *binaryOutputFilename;
    bool importBinary;
    const char *binaryInputFilename;
    int iterations; // Radiance method iterations
    const char *radianceImageFileNameFormat;
    const char *radianceModelFileNameFormat;
    int saveModulo; // Every n-th iteration the image will be saved (the model at every iteration)
    int saveLastOnly; // Image and model saved only after the last iteration (overrides saveModulo)
    const char *raytracingImageFileName;
    int timings = false;

    BatchOptions();
    virtual ~BatchOptions();
};

#endif
