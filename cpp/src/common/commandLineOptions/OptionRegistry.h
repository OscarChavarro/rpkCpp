#ifndef __OPTION_REGISTRY__
#define __OPTION_REGISTRY__

#include "common/commandLineOptions/TypedOption.h"

template<typename T>
class OptionRegistry {
  public:
    OptionRegistry():
        options_(nullptr),
        count_(0) {
    }

    OptionRegistry(TypedOption<T> *options, int count):
        options_(options),
        count_(count) {
    }

    TypedOption<T> *options() const {
        return options_;
    }

    int count() const {
        return count_;
    }

  private:
    TypedOption<T> *options_;
    int count_;
};

#endif
