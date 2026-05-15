package vsdk.toolkit.common.statistics;

public class ReaderStatistics {
    public int numberOfGeometries;
    public int numberOfCompounds;
    public int numberOfSurfaces;
    public int numberOfVertices;
    public int numberOfPatches;
    public int numberOfElements;
    public int numberOfLightSources;

    public ReaderStatistics() {
        numberOfGeometries = 0;
        numberOfCompounds = 0;
        numberOfSurfaces = 0;
        numberOfVertices = 0;
        numberOfPatches = 0;
        numberOfElements = 0;
        numberOfLightSources = 0;
    }

    public void reset() {
        numberOfGeometries = 0;
        numberOfCompounds = 0;
        numberOfSurfaces = 0;
        numberOfVertices = 0;
        numberOfPatches = 0;
        numberOfElements = 0;
        numberOfLightSources = 0;
    }
}
