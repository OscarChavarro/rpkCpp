/**
Rendering stuff independent of the graphics library being used
*/

#include "java/util/ArrayList.txx"
#include "render/opengl.h"
#include "render/render.h"

/**
Computes front- and back-clipping plane distance for the current GLOBAL_scene_world and
camera
*/
void
renderGetNearFar(Camera *camera, const java::ArrayList<Geometry *> *sceneGeometries) {
    BoundingBox bounds;
    Vector3D b[2];
    Vector3D d;

    if ( sceneGeometries == nullptr || sceneGeometries->size() == 0 ) {
        camera->far = 10.0f;
        camera->near = 0.1f;
        return;
    }

    geometryListBounds(sceneGeometries, &bounds);

    b[0].set(bounds.coordinates[MIN_X], bounds.coordinates[MIN_Y], bounds.coordinates[MIN_Z]);
    b[1].set(bounds.coordinates[MAX_X], bounds.coordinates[MAX_Y], bounds.coordinates[MAX_Z]);

    camera->far = -Numeric::HUGE_FLOAT_VALUE;
    camera->near = Numeric::HUGE_FLOAT_VALUE;
    for ( int i = 0; i <= 1; i++ ) {
        for ( int j = 0; j <= 1; j++ ) {
            for ( int k = 0; k <= 1; k++ ) {
                d.set(b[i].x, b[j].y, b[k].z);
                d.subtraction(d, camera->eyePosition);
                float z = d.dotProduct(camera->Z);

                if ( z > camera->far ) {
                    camera->far = z;
                }
                if ( z < camera->near ) {
                    camera->near = z;
                }
            }
        }
    }

    // Take 2% extra distance for near as well as far clipping plane
    camera->far += 0.02f * (camera->far);
    camera->near -= 0.02f * (camera->near);
    if ( camera->far < Numeric::EPSILON ) {
        camera->far = camera->viewDistance;
    }
    if ( camera->near < Numeric::EPSILON ) {
        camera->near = camera->viewDistance / 100.0f;
    }
}

/**
Renders a bounding box
*/
void
renderBoundingBox(BoundingBox boundingBox) {
    Vector3D p[8];

    p[0].set(boundingBox.coordinates[MIN_X], boundingBox.coordinates[MIN_Y], boundingBox.coordinates[MIN_Z]);
    p[1].set(boundingBox.coordinates[MAX_X], boundingBox.coordinates[MIN_Y], boundingBox.coordinates[MIN_Z]);
    p[2].set(boundingBox.coordinates[MIN_X], boundingBox.coordinates[MAX_Y], boundingBox.coordinates[MIN_Z]);
    p[3].set(boundingBox.coordinates[MAX_X], boundingBox.coordinates[MAX_Y], boundingBox.coordinates[MIN_Z]);
    p[4].set(boundingBox.coordinates[MIN_X], boundingBox.coordinates[MIN_Y], boundingBox.coordinates[MAX_Z]);
    p[5].set(boundingBox.coordinates[MAX_X], boundingBox.coordinates[MIN_Y], boundingBox.coordinates[MAX_Z]);
    p[6].set(boundingBox.coordinates[MIN_X], boundingBox.coordinates[MAX_Y], boundingBox.coordinates[MAX_Z]);
    p[7].set(boundingBox.coordinates[MAX_X], boundingBox.coordinates[MAX_Y], boundingBox.coordinates[MAX_Z]);

    openGlRenderLine(&p[0], &p[1]);
    openGlRenderLine(&p[1], &p[3]);
    openGlRenderLine(&p[3], &p[2]);
    openGlRenderLine(&p[2], &p[0]);
    openGlRenderLine(&p[4], &p[5]);
    openGlRenderLine(&p[5], &p[7]);
    openGlRenderLine(&p[7], &p[6]);
    openGlRenderLine(&p[6], &p[4]);
    openGlRenderLine(&p[0], &p[4]);
    openGlRenderLine(&p[1], &p[5]);
    openGlRenderLine(&p[2], &p[6]);
    openGlRenderLine(&p[3], &p[7]);
}

static void
renderGeomBounds(Camera *camera, const Geometry *geometry) {
    BoundingBox geometryBoundingBox = geometry->getBoundingBox();

    if ( geometry->bounded ) {
        renderBoundingBox(geometryBoundingBox);
    }

    if ( geometry->isCompound() ) {
        java::ArrayList<Geometry *> *geometryList = geomPrimListCopy(geometry);
        for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
            renderGeomBounds(camera, geometryList->get(i));
        }
        delete geometryList;
    }
}

/**
Renders the bounding boxes of all objects in the scene
*/
void
renderBoundingBoxHierarchy(Camera *camera, const java::ArrayList<Geometry *> *sceneGeometries, const RenderOptions *renderOptions) {
    openGlRenderSetColor(&renderOptions->boundingBoxColor);
    for ( int i = 0; sceneGeometries != nullptr && i < sceneGeometries->size(); i++ ) {
        renderGeomBounds(camera, sceneGeometries->get(i));
    }
}

/**
Renders the cluster hierarchy bounding boxes
*/
void
renderClusterHierarchy(Camera *camera, const java::ArrayList<Geometry *> *clusteredGeometryList, const RenderOptions *renderOptions) {
    openGlRenderSetColor(&renderOptions->clusterColor);
    for ( int i = 0; clusteredGeometryList != nullptr && i < clusteredGeometryList->size(); i++ ) {
        renderGeomBounds(camera, clusteredGeometryList->get(i));
    }
}
