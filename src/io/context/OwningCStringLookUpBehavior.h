#ifndef __OWNING_CSTRING_LOOKUP_BEHAVIOR__
#define __OWNING_CSTRING_LOOKUP_BEHAVIOR__

#include "io/context/StringLookUpBehavior.h"

class OwningCStringLookUpBehavior : public StringLookUpBehavior {
  public:
    void
    freeKey(const char *key) const override;

    void
    freeData(const char *data) const override;
};

#endif
