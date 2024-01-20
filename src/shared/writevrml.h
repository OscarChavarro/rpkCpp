/*
Saves the result of a radiosity computation as a VRML file.
*/

#ifndef _WRITE_VRML_H_
#define _WRITE_VRML_H_

#include <cstdio>

#include "common/linealAlgebra/Matrix4x4.h"
#include "shared/Camera.h"

/* Default method for saving VRML models (if the current radiance method
 * doesn't have its own one. */
extern
void WriteVRML(FILE *fp);

/* Can also be used by radiance-method specific VRML savers. See
 * WriteVRML.c for how they are to be used and what they do. */
extern
void WriteVRMLHeader(FILE *fp);

extern
void WriteVRMLTrailer(FILE *fp);

extern
Matrix4x4 VRMLModelTransform(Vector3D *model_rotaxis, float *model_rotangle);

extern
void WriteVRMLViewPoint(FILE *fp, Matrix4x4 model_xf, Camera *cam, const char *vpname);

extern
void
WriteVRMLViewPoints(FILE *fp, Matrix4x4 model_xf);

#endif
