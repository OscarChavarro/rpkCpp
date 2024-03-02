#ifndef __LIST__
#define __LIST__

class LIST {
  public:
    void *pelement;
    LIST *next;
};

#ifndef EndForAll
#define EndForAll }}}
#endif

extern LIST *ListAdd(LIST *list, void *element);
extern LIST *ListRemove(LIST *list, void *pelement);
extern void ListDestroy(LIST *list);

#endif
