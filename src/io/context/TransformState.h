#ifndef __TRANSFORM_STATE__
#define __TRANSFORM_STATE__

class TransformStackContext;

class TransformState {
  public:
    TransformStackContext *transformContext;

    TransformState();
};

#endif
