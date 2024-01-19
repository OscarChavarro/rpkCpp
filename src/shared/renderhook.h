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
void RenderHooks();

/* Add a hook */

void AddRenderHook(RENDERHOOKFUNCTION func, void *data);

/* Remove a hook, function AND data must match
   those used in the corresponding AddRenderHook call
 */

void RemoveRenderHook(RENDERHOOKFUNCTION func, void *data);

void removeAllRenderHooks();

#endif
