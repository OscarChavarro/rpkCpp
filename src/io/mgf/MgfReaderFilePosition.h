#ifndef __MGF_READER_FILE_POSITION__
#define __MGF_READER_FILE_POSITION__

class MgfReaderFilePosition {
public:
    int fid; // File this position is for
    int lineno; // Line number in file
    long offset; // Offset from beginning
};

#endif
