#ifndef __LOOKUP_ENTITY__
#define __LOOKUP_ENTITY__

class LookUpEntity {
  public:
    LookUpEntity();

    char *key; // Key name
    long value; // Key hash value (for efficiency)
    char *data; // Client data
};

#endif
