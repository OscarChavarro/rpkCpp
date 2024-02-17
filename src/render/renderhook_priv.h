#ifndef _RENDERHOOK_PRIV_H_
#define _RENDERHOOK_PRIV_H_

#include "common/dataStructures/List.h"
#include "render/renderhook.h"

class RENDERHOOK {
public:
    RENDERHOOKFUNCTION func;
    void *data;
};

#endif
