#ifndef __STRING_LOOK_UP_BEHAVIOR__
#define __STRING_LOOK_UP_BEHAVIOR__

#include "io/context/LookUpBehavior.h"

class StringLookUpBehavior : public LookUpBehavior {
  public:
    long
    hash(const char *key) const override;

    bool
    keysEqual(const char *left, const char *right) const override;
};

#endif
