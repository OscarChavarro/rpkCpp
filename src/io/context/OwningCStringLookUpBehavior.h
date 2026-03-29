#ifndef __OWNING_CSTRING_LOOKUP_BEHAVIOR__
#define __OWNING_CSTRING_LOOKUP_BEHAVIOR__

#include "io/context/CStringLookUpBehavior.h"

class OwningCStringLookUpBehavior : public CStringLookUpBehavior {
  public:
    void
    freeKey(const char *key) const override;

    void
    freeData(const char *data) const override;
};

#endif
