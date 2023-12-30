/* patch_flags.h: extra PATCH flags (stored in PATCH.flags field) */

#ifndef _PATCH_FLAGS_H_
#define _PATCH_FLAGS_H_

#define PATCH_VISIBILITY    0x01
#define PATCH_IS_VISIBLE(patch)    (((patch)->flags & PATCH_VISIBILITY)!=0)
#define PATCH_SET_VISIBLE(patch)    {(patch)->flags |= PATCH_VISIBILITY;}
#define PATCH_SET_INVISIBLE(patch)    {(patch)->flags &= ~PATCH_VISIBILITY;}

#endif
