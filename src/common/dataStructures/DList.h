/* list.h: generic doubly linked linear lists */

#ifndef _DLIST_H_
#define _DLIST_H_

class DLIST {
  public:
    void *pelement;        /* pointer to an element */
    DLIST *prev, *next;    /* pointer to the rest of the dlist */
};

/* some of the DList precodures are implemented both as a macro and as a
 * C function. Define NODLISTMACROS to use the C-function implementation. Macros
 * have the advantage of avoiding a function call however. These routines are
 * so often used that you really save time by using the macros. */
/* #define NODLISTMACROS */

/*
 * iterator: iterates over all elements in the list 'list', containing
 * pointers to elements of type 'TYPE'. The elements are named 'p'.
 * Use this as follows:
 * 
 * ForAllInDList(PATCH, P, patches) {
 *  do something with 'P';
 * } EndForAll;
 *
 * PS: you can make things even nicer if you define macro's such as
 * #define ForAllPatches(p, patches) ForAllInDList(PATCH, p, patches)
 * You can than use
 * ForAllPatches(P, patches) {
 *  do something with 'P';
 * } EndForAll;
 */
#define ForAllInDList(TYPE, p, list) {        \
  DLIST *_list_ = (DLIST *)(list);        \
  if (_list_) {                    \
    DLIST *_l_;                    \
    for (_l_ = _list_; _l_; _l_ = _l_->next) {    \
      TYPE *p = (TYPE *)(_l_->pelement);

#ifndef EndForAll    /* all definitions of EndForAll are the same */
#define EndForAll }}}
#endif

/* adds an element in front of the dlist, returns a pointer to the new dlist */
extern DLIST *DListAdd(DLIST *dlist, void *element);

#endif
