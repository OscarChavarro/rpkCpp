#ifndef __LOOKUP_BEHAVIOR__
#define __LOOKUP_BEHAVIOR__

class LookUpBehavior {
  public:
    virtual ~LookUpBehavior() {}

    virtual long
    hash(const char *key) const = 0;

    virtual bool
    keysEqual(const char *left, const char *right) const = 0;

    virtual void
    freeKey(const char * /*key*/) const {}

    virtual void
    freeData(const char * /*data*/) const {}
};

#endif
