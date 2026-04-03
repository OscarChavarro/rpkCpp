#ifndef __TRANSFORM_STACK__
#define __TRANSFORM_STACK__

#include "io/context/TransformStackContext.h"

class TransformStack {
  public:
    TransformStackContext *transformContext;
    char **argumentList;
    int argumentCount;
    char iterateArgument[3];

    TransformStack();
    ~TransformStack();

    void clearArguments();
    int argumentCountFor(const TransformStackContext *context) const;
    int argumentStartIndexFor(const TransformStackContext *context) const;
    char **argumentVectorFor(const TransformStackContext *context) const;
    bool compactTo(const TransformStackContext *context);
    void freeTransformContext(TransformStackContext *context) const;

    TransformStack(const TransformStack &) = delete;
    TransformStack &operator=(const TransformStack &) = delete;
};

#endif
