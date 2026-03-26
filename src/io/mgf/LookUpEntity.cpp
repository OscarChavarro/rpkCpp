#include "io/mgf/LookUpEntity.h"

LookUpEntity::LookUpEntity():
    key(nullptr),
    value(0),
    data(nullptr)
{
}

LookUpEntity::LookUpEntity(char *key, long value, char *data):
    key(key),
    value(value),
    data(data)
{
}
