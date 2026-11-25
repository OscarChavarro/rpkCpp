#ifndef __FILE_POSITION_CONTEXT__
#define __FILE_POSITION_CONTEXT__

class FilePositionContext {
  public:
    int fileId; // File this position is for
    int lineNumber; // Line number in file
    long offset; // Offset from beginning
};

#endif
