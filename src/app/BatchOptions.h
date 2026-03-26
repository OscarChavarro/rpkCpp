#ifndef RPK_BATCHOPTIONS_H
#define RPK_BATCHOPTIONS_H

class BatchOptions {
  public:
    bool exportBinary;
    const char *binaryOutputFilename;
    bool importBinary;
    const char *binaryInputFilename;
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
