#ifndef _VERTEXLIST_H_
#define _VERTEXLIST_H_

#include "common/dataStructures/List.h"
#include "skin/Vertex.h"

class VERTEX;

class VERTEXLIST {
  public:
    VERTEX *vertex;
    VERTEXLIST *next;
};

#define VertexListAdd(vertexlist, vertex) \
        (VERTEXLIST *)ListAdd((LIST *)vertexlist, (void *)vertex)

#define VertexListIterate(vertexlist, proc) \
        ListIterate((LIST *)vertexlist, (void (*)(void *))proc)

#define VertexListDestroy(vertexlist) \
        ListDestroy((LIST *)vertexlist)

#endif
