#ifndef __RAY_CORPUS_READER__
#define __RAY_CORPUS_READER__

#include <stdio.h>
#include "VisibilityQueryRecord.h"

/**
Sequential reader of a ray corpus file written by VisibilityQueryRecorder
(format in its header). GEOMETRY_LIST records are skipped and counted: the
restricted-list queries they precede are not index queries, and the shaft
culling patch sets they name exist only during the run that wrote them.

C++ port note: the C++ port skips a geometry list with a relative fseek; this
port reads it through, so that the reader never seeks or asks for a file
position, and a corpus beyond 2 GB (or 4 GB) needs only sequential reads,
never a 64-bit offset. The geometry list counter is a double holding a whole
number (exact up to 2^53), since Turbo C++ 3.0 has no 64-bit integer.
*/
class RayCorpusReader {
  private:
    FILE *file;
    char *buffer;
    double geometryLists;
    bool valid;

    // Not copyable
    RayCorpusReader(const RayCorpusReader &);
    RayCorpusReader &operator=(const RayCorpusReader &);

  public:
    explicit RayCorpusReader(const char *fileName);
    ~RayCorpusReader();

    /** True when the file opened and carries the RPKVQC01 version 1 header */
    bool isValid() const { return valid; }

    /**
    Reads the next QUERY record.
    @return false at the end of the file, or on a malformed one (isValid
    then turns false)
    */
    bool next(VisibilityQueryRecord *record);

    double getGeometryLists() const { return geometryLists; }
};

#endif
