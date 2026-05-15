#ifndef __CONTRIB_HANDLER__
#define __CONTRIB_HANDLER__

#include "common/color/ColorRgb.h"
#include "raycasting/bidirectionalRaytracing/BiPath.h"
#include "raycasting/bidirectionalRaytracing/FlagChainList.h"

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
    virtual ColorRgb compute(BiPath *path);

  protected:
    virtual void doRegExp(char *regExp, bool subtract);
    void doSyntaxError(const char *errString);
    bool getFlags(const char *regExp, int *pos, char *flags);
    bool getToken(const char *regExp, int *pos, char *token, char *flags);
    void doRegExpGeneral(const char *regExp, bool subtract);
};

#endif
