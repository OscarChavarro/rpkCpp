#ifndef RAY_CORPUS_READER__
#define RAY_CORPUS_READER__

#include <cstdio>
#include "VisibilityQueryRecord.h"

/**
Sequential reader of a ray corpus file written by VisibilityQueryRecorder
(format in its header). GEOMETRY_LIST records are skipped and counted: the
restricted-list queries they precede are not index queries, and the shaft
culling patch sets they name exist only during the run that wrote them.
*/
class RayCorpusReader {
  private:
    FILE *file;
    char *buffer;
    long long geometryLists;
    bool valid;

  public:
    explicit RayCorpusReader(const char *fileName);
    ~RayCorpusReader();
    RayCorpusReader(const RayCorpusReader &) = delete;
    RayCorpusReader &operator=(const RayCorpusReader &) = delete;

    /** True when the file opened and carries the RPKVQC01 version 1 header */
    bool isValid() const { return valid; }

    /**
    Reads the next QUERY record.
    @return false at the end of the file, or on a malformed one (isValid
    then turns false)
    */
    bool next(VisibilityQueryRecord *record);

    long long getGeometryLists() const { return geometryLists; }
};

#endif
