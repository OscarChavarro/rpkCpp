/* stackmac.h: simple stack macros */

#ifndef _STACKMAC_H_
#define _STACKMAC_H_

#define STACK_DECL(TYPE, stack, maxdepth, stackptr)    \
TYPE stack[maxdepth+1]; TYPE *stackptr = &stack[0]

#define STACK_SAVE(obj, stack, maxdepth, stackptr) {    \
  if (stackptr-stack < maxdepth)            \
    *stackptr++ = obj;                    \
  else                            \
    logFatal(-1, "STACK_SAVE", "Stack overflow in %s line %d", __FILE__, __LINE__); \
}

#define STACK_RESTORE_NOCHECK(obj, stack, stackptr) {    \
  obj = *--stackptr;                    \
}

#endif
