#ifndef __LIST__
#define __LIST__

class LIST {
  public:
    void *pelement;
    LIST *next;
};

/**
Iterator: iterates over all elements in the list 'list', containing
pointers to elements of type 'TYPE'. The elements are named 'p'.
Use this as follows:

ForAllInList(Patch, P, patches) {
 do something with 'P';
} EndForAll;

PS: you can make things even nicer if you define macro's such as
#define ForAllPatches(p, patches) ForAllInList(Patch, p, patches)
You can than use
ForAllPatches(P, patches) {
 do something with 'P';
} EndForAll;
*/
#define ForAllInList(TYPE, p, list) { \
    LIST *listStart = (LIST *)(list); \
    if ( listStart != nullptr ) { \
        for ( LIST *window = listStart; window != nullptr; window = window->next ) { \
            TYPE *p = (TYPE *)(window->pelement);

#ifndef EndForAll
#define EndForAll }}}
#endif

/**
Iterators: executes the procedure for each element of the list.
There are a number of iterators here: use ListIterate with
a procedure that accepts only one parameter: a pointer to an element
*/
#define ListIterate(list, proc) \
{ \
    LIST *window = (list); \
    while ( window != nullptr ) { \
        void *pelement = window->pelement; \
        window = window->next; \
        ((void (*)(void *))proc)(pelement); \
    } \
}

extern LIST *ListAdd(LIST *list, void *element);
extern LIST *ListRemove(LIST *list, void *pelement);
extern void ListDestroy(LIST *list);

#endif
