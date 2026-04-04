package vsdk.toolkit.app.options;

public class BatchOptions {
    public boolean exportBinary;
    public String binaryOutputFilename;
    public boolean importBinary;
    public String binaryInputFilename;
    public int iterations; // Radiance method iterations
    public String radianceImageFileNameFormat;
    public String radianceModelFileNameFormat;
    public int saveModulo; // Every n-th iteration, surface model and image will be saved
    public String raytracingImageFileName;
    public int timings = 0;

    public BatchOptions() {
        exportBinary = false;
        binaryOutputFilename = "";
        importBinary = false;
        binaryInputFilename = "";
        iterations = 1;
        radianceImageFileNameFormat = "";
        radianceModelFileNameFormat = "";
        saveModulo = 10;
        raytracingImageFileName = "";
        timings = 0;
    }
}
