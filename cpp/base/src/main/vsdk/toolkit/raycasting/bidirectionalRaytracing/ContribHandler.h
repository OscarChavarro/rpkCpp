#ifndef CONTRIB_HANDLER__
#define CONTRIB_HANDLER__

#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/BiPath.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/FlagChainList.h"

/**
An array of chain lists indexed by length
*/
class ContribHandler {
  public:
    FlagChainList *array;
    int maxLength;

    ContribHandler();
    virtual void init(int paramMaxLength);
    virtual ~ContribHandler();
    virtual void addRegExp(char *regExp);
    virtual ColorRgbMutable compute(BiPath *path);

  protected:
    virtual void doRegExp(char *regExp, bool subtract);
    void doSyntaxError(const char *errString);
    bool getFlags(const char *regExp, int *pos, char *flags);
    bool getToken(const char *regExp, int *pos, char *token, char *flags);
    void doRegExpGeneral(const char *regExp, bool subtract);
};

#endif
