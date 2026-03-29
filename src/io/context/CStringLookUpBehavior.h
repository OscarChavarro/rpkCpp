#ifndef __CSTRING_LOOKUP_BEHAVIOR__
#define __CSTRING_LOOKUP_BEHAVIOR__

#include "io/context/LookUpBehavior.h"

class CStringLookUpBehavior : public LookUpBehavior {
  public:
    long
    hash(const char *key) const override;

    bool
    keysEqual(const char *left, const char *right) const override;
};

#endif
