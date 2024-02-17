#ifndef _DLIST_H_
#define _DLIST_H_

class DLIST {
  public:
    void *element;
    DLIST *next;
};

#ifndef EndForAll
#define EndForAll }}}
#endif

extern DLIST *listAdd(DLIST *list, void *element);

#endif
