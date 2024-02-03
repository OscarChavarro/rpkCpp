/* renderhooklist.h: a list of functions called when rendering the scene */

#ifndef _RENDERHOOKLIST_H_
#define _RENDERHOOKLIST_H_

/* Render hooks are called each time the scene is rendered.
   Functions are provided to add and remove hooks.
   Hooks should only depend on render.h, not on GLX or OpenGL.
*/

/* Callback definition */

typedef void (*RENDERHOOKFUNCTION)(void *data);

/* Renders the hooks, called from render.c */
void renderHooks();

/* Add a hook */

void addRenderHook(RENDERHOOKFUNCTION func, void *data);

/* Remove a hook, function AND data must match
   those used in the corresponding addRenderHook call
 */

void removeRenderHook(RENDERHOOKFUNCTION func, void *data);

void removeAllRenderHooks();

#endif
