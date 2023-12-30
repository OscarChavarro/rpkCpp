/* rmoptions.H: Options and global state vars for raymatting */

#ifndef _RMOPTIONS_H_
#define _RMOPTIONS_H_

#include "raycasting/common/ScreenBuffer.h"

/*** Typedefs & enums ***/

enum RM_FILTER_OPTION {
    RM_BOX_FILTER,
    RM_TENT_FILTER,
    RM_GAUSS_FILTER,
    RM_GAUSS2_FILTER
};

/*** The global state structure ***/

class RM_State {
  public:
    /* Pixel sampling */
    int samplesPerPixel;

    /* pixel filter */
    RM_FILTER_OPTION filter;
};


/*** The global state ***/

extern RM_State rms;


/*** Function prototypers ***/

void RM_Defaults();

void RM_ParseOptions(int *argc, char **argv);

void RM_PrintOptions(FILE *fp);

#endif
