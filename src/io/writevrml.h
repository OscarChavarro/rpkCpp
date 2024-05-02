/**
Saves the result of a radiosity computation as a VRML file
*/

#ifndef __WRITE_VRML__
#define __WRITE_VRML__

#include "java/util/ArrayList.h"
#include "common/RenderOptions.h"
#include "skin/Patch.h"
#include "scene/Camera.h"

extern void writeVRML(const Camera *camera, FILE *fp, const java::ArrayList<Patch *> *scenePatches);
extern void writeVrmlHeader(const Camera *camera, FILE *fp, const RenderOptions *renderOptions);
extern void writeVRMLTrailer(FILE *fp);
extern Matrix4x4 transformModelVRML(const Camera *camera, Vector3D *modelRotationAxis, float *modelRotationAngle);
extern void writeVRMLViewPoints(const Camera *camera, FILE *fp, const Matrix4x4 *modelTransform);

#endif
