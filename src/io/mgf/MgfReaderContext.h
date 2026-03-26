#ifndef __MGF_READER_CONTEXT__
#define __MGF_READER_CONTEXT__

inline constexpr int MGF_MAXIMUM_INPUT_LINE_LENGTH = 4096;
inline constexpr int MGF_MAXIMUM_ARGUMENT_COUNT = (MGF_MAXIMUM_INPUT_LINE_LENGTH / 4);

namespace java {
namespace io {
class BufferedInputStream;
}
}

class MgfReaderContext {
  public:
    char fileName[96];
    java::io::BufferedInputStream *inputStream; // stream pointer
    int fileContextId;
    char inputLine[MGF_MAXIMUM_INPUT_LINE_LENGTH];
    int lineNumber;
    char isPipe; // Flag indicating whether input comes from a pipe or a real file
    MgfReaderContext *prev; // Previous context
};

#endif
