/**
Saves the result of a radiosity computation as a VRML file
*/

#ifndef __WRITE_VRML__
#define __WRITE_VRML__

#include "scene/Camera.h"

extern void writeVRML(FILE *fp, java::ArrayList<Patch *> *scenePatches);
extern void writeVrmlHeader(FILE *fp);
extern void writeVRMLTrailer(FILE *fp);
extern Matrix4x4 transformModelVRML(Vector3D *modelRotationAxis, float *modelRotationAngle);
extern void writeVRMLViewPoints(FILE *fp, Matrix4x4 model_xf);

#endif
