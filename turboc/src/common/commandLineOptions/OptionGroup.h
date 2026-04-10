#ifndef __OPTION_CORE_GROUP__
#define __OPTION_CORE_GROUP__

#include "common/commandLineOptions/TypedOption.h"

template<typename TOptionBase>
class OptionGroupT {
  public:
    OptionGroupT();
    OptionGroupT(const char *groupName, TOptionBase *groupOptions, int groupCount);

    const char *name;
    TOptionBase *options;
    int count;
};

typedef OptionGroupT<OptionBase> OptionGroup;

#include "common/commandLineOptions/OptionGroup.txx"

#endif
