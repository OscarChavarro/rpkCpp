#ifndef READER_STATISTICS__
#define READER_STATISTICS__

class ReaderStatistics {
  public:
    int numberOfGeometries;
    int numberOfCompounds;
    int numberOfSurfaces;
    int numberOfVertices;
    int numberOfPatches;
    int numberOfElements;
    int numberOfLightSources;

    ReaderStatistics();
    void reset();
};

#endif
