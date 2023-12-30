#ifndef _LIST_H_
#define _LIST_H_

class LIST {
  public:
    void *pelement;    /* pointer to an element */
    LIST *next;    /* pointer to the rest of the list */
};

/* some of the List precodures are implemented both as a macro and as a
 * C function. Define NOLISTMACROS to use the C-function implementation. Macros
 * have the advantage of avoiding a function call however. These routines are
 * so often used that you really save time by using the macros. */
/* #define NOLISTMACROS */

/* creates an empty list */
#define ListCreate()  ((LIST *)nullptr)

/*
 * iterator: iterates over all elements in the list 'list', containing
 * pointers to elements of type 'TYPE'. The elements are named 'p'.
 * Use this as follows:
 * 
 * ForAllInList(PATCH, P, patches) {
 *  do something with 'P';
 * } EndForAll;
 *
 * PS: you can make things even nicer if you define macro's such as
 * #define ForAllPatches(p, patches) ForAllInList(PATCH, p, patches)
 * You can than use
 * ForAllPatches(P, patches) {
 *  do something with 'P';
 * } EndForAll;
 */
#define ForAllInList(TYPE, p, list) { \
  LIST *_list_ = (LIST *)(list); \
  if (_list_) { \
    LIST *_l_; \
    for (_l_ = _list_; _l_; _l_ = _l_->next) { \
      TYPE *p = (TYPE *)(_l_->pelement);

#ifndef EndForAll
#define EndForAll }}}
#endif

/* iterators: executes the procedure for each element of the list.
 * There are a number of iterators here: use ListIterate with
 * a procedure that accepts only one parameter: a pointer to an element */
#define ListIterate(list, proc) \
{ \
    LIST *_list = (list); \
    void *pelement; \
    while (_list) { \
        pelement = _list->pelement; \
        _list = _list->next; \
        ((void (*)(void *))proc)(pelement); \
    } \
}

/* use ListIterate1A with procedures that accepts two parameters: first
 * a pointer to the list element, then a pointer to the "extra" data */
#define ListIterate1A(list, proc, extra)                \
{                                    \
    LIST *_list = (list);                        \
    void *pelement;                            \
    while (_list) {                            \
        pelement = _list->pelement;                \
        _list = _list->next;                    \
        ((void (*)(void *, void *))proc)(pelement, (void *)(extra));\
    }                                \
}

/* use ListIterate1B with procedures that accepts two parameters: first
 * a pointer to the "extra" data, then a pointer to a list element */
#define ListIterate1B(list, proc, extra)                \
{                                    \
    LIST *_list = (list);                        \
    void *pelement;                            \
    while (_list) {                            \
        pelement = _list->pelement;                \
        _list = _list->next;                    \
        ((void (*)(void *, void *))proc)((void *)(extra), pelement);\
    }                                \
}

/* adds an element in front of the list, returns a pointer to the new list */
extern LIST *ListAdd(LIST *list, void *element);

/* counts the number of elements in a list */
extern int ListCount(LIST *list);

/* the first argument is the adres of a LIST *. First call this function
 * with the adres to the LIST * being the adres of a pointer to the first 
 * element of the list. By calling this function successively, a
 * pointer to each element of the list is returned in sequence. 
 * This function returns nullptr after the last element of the list
 * has been encountered. 
 *
 * WARNING: ListNext() calls cannot be nested! */
extern void *__plistel__;
#define ListNext(plist) ((*(plist)) ? (__plistel__=(*(plist))->pelement, (*(plist))=(*(plist))->next, __plistel__) : (void *)nullptr)

/* merge two lists: the elements of list2 are prepended to the elements
 * of list1. Returns a pointer to the enlarged list1 */
extern LIST *ListMerge(LIST *list1, LIST *list2);

/* duplicates a list: the elements are not duplicated: only a pointer
 * to the elements is copied. */
extern LIST *ListDuplicate(LIST *list);

/* removes an element from the list. Returns a pointer to the updated
 * list. */
extern LIST *ListRemove(LIST *list, void *pelement);

// destroys a list. Does not destroy the elements
extern void ListDestroy(LIST *list);

#endif
