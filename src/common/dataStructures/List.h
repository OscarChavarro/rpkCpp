#ifndef _LIST_H_
#define _LIST_H_

class LIST {
  public:
    void *pelement;    /* pointer to an element */
    LIST *next;    /* pointer to the rest of the list */
};

// Creates an empty list
#define ListCreate()  ((LIST *)nullptr)

/**
iterator: iterates over all elements in the list 'list', containing
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

/**
Use ListIterate1A with procedures that accepts two parameters: first
a pointer to the list element, then a pointer to the "extra" data
*/
#define ListIterate1A(list, proc, extra) \
{ \
    for ( LIST *window = (list); window != nullptr; window = window->next ) { \
        void *pelement = window->pelement; \
        ((void (*)(void *, void *))proc)(pelement, (void *)(extra)); \
    } \
}

/**
Use ListIterate1B with procedures that accepts two parameters: first
a pointer to the "extra" data, then a pointer to a list element
*/
#define ListIterate1B(list, proc, extra) \
{ \
    for ( LIST *window = (list); window != nullptr; window = window->next ) { \
        void *pelement = window->pelement; \
        ((void (*)(void *, void *))proc)((void *)(extra), pelement); \
    } \
}


extern LIST *ListAdd(LIST *list, void *element);

/**
The first argument is the address of a LIST *. First call this function
with the address to the LIST * being the address of a pointer to the first
element of the list. By calling this function successively, a
pointer to each element of the list is returned in sequence.
This function returns nullptr after the last element of the list
has been encountered.

WARNING: ListNext() calls cannot be nested!
*/
extern void *GLOBAL_listHandler;
#define ListNext(plist) ((*(plist)) ? (GLOBAL_listHandler=(*(plist))->pelement, (*(plist))=(*(plist))->next, GLOBAL_listHandler) : (void *)nullptr)

extern LIST *ListRemove(LIST *list, void *pelement);
extern void ListDestroy(LIST *list);

#endif
