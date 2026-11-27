#ifndef __OPTION_GROUP__
#define __OPTION_GROUP__

#include "vsdk/toolkit/common/commandLineOptions/TypedOption.h"

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

#include "vsdk/toolkit/common/commandLineOptions/OptionGroup.txx"

#endif
