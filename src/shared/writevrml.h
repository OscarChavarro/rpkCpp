/**
Saves the result of a radiosity computation as a VRML file
*/

#ifndef __WRITE_VRML__
#define __WRITE_VRML__

#include "shared/Camera.h"

extern void writeVRML(FILE *fp);
extern void writeVrmlHeader(FILE *fp);
extern void writeVRMLTrailer(FILE *fp);
extern Matrix4x4 transformModelVRML(Vector3D *model_rotaxis, float *model_rotangle);
extern void writeVRMLViewPoint(FILE *fp, Matrix4x4 model_xf, Camera *cam, const char *vpname);
extern void writeVRMLViewPoints(FILE *fp, Matrix4x4 model_xf);

#endif
