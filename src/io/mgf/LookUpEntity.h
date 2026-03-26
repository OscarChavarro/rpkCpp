#ifndef __LOOKUP_ENTITY__
#define __LOOKUP_ENTITY__

class LookUpEntity {
  public:
    LookUpEntity();
    LookUpEntity(char *key, long value, char *data);

    char *key; // Key name
    long value; // Key hash value (for efficiency)
    char *data; // Client data
};

#endif
