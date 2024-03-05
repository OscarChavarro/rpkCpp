#ifndef __LIST__
#define __LIST__

class LIST {
  public:
    void *pelement;
    LIST *next;
};

extern LIST *listAdd(LIST *list, void *element);
extern LIST *listRemove(LIST *list, void *element);
extern void listDestroy(LIST *list);

#endif
