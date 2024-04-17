/**
Saves the result of a radiosity computation as a VRML file
*/

#ifndef __WRITE_VRML__
#define __WRITE_VRML__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"
#include "scene/Camera.h"

extern void writeVRML(Camera *camera, FILE *fp, java::ArrayList<Patch *> *scenePatches);
extern void writeVrmlHeader(Camera *camera, FILE *fp);
extern void writeVRMLTrailer(FILE *fp);
extern Matrix4x4 transformModelVRML(Camera *camera, Vector3D *modelRotationAxis, float *modelRotationAngle);
extern void writeVRMLViewPoints(Camera *camera, FILE *fp, Matrix4x4 model_xf);

#endif
