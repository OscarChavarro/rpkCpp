/**
Ray casting using the SGL library for rendering Patch pointers into
a software frame buffer directly.
*/

#include <ctime>

#include "common/error.h"
#include "material/statistics.h"
#include "shared/SoftIdsWrapper.h"
#include "raycasting/simple/RayCaster.h"

void
RayCaster::clipUv(int nrvertices, double *u, double *v) {
    if ( *u > 1. - EPSILON ) {
        *u = 1. - EPSILON;
    }
    if ( *v > 1. - EPSILON ) {
        *v = 1. - EPSILON;
    }
    if ( nrvertices == 3 && (*u + *v) > 1. - EPSILON ) {
        if ( *u > *v ) {
            *u = 1. - *v - EPSILON;
        } else {
            *v = 1. - *u - EPSILON;
        }
    }
    if ( *u < EPSILON ) {
        *u = EPSILON;
    }
    if ( *v < EPSILON ) {
        *v = EPSILON;
    }
}

void
RayCaster::render(GETRADIANCE_FT getrad = nullptr) {
    clock_t t = clock();
    interrupt_requested = false;

    if ( getrad == nullptr ) {
        if ( GLOBAL_radiance_currentRadianceMethodHandle ) {
            getrad = GLOBAL_radiance_currentRadianceMethodHandle->GetRadiance;
        }
    }

    long width, height, x, y;
    Soft_ID_Renderer *id_renderer = new Soft_ID_Renderer;
    id_renderer->get_size(&width, &height);
    if ( width != scrn->getHRes() ||
         height != scrn->getVRes()) {

        logFatal(-1, "RayCaster::render", "ID buffer size doesn't match screen size");
    }

    // TODO SITHMASTER: This is the main paralelizable loop for raycasting
    for ( y = 0; y < height; y++ ) {
        for ( x = 0; x < width; x++ ) {
            Patch *P = id_renderer->get_patch_at_pixel(x, y);
            COLOR rad = getRadianceAtPixel(x, y, P, getrad);
            scrn->add(x, y, rad);
        }

        scrn->renderScanline(y);
        if ( interrupt_requested ) {
            break;
        }
    }

    delete id_renderer;

    GLOBAL_raytracer_totalTime = (float) (clock() - t) / (float) CLOCKS_PER_SEC;
    GLOBAL_raytracer_rayCount = GLOBAL_raytracer_pixelCount = 0;
}

void
RayCaster::display() {
    scrn->render();
}

void
RayCaster::save(ImageOutputHandle *ip) {
    scrn->writeFile(ip);
}

void
RayCaster::interrupt() {
    interrupt_requested = true;
}

static RayCaster *s_rc = nullptr;

/**
Returns false if there is no previous image and true if there is
*/
static int
Redisplay() {
    if ( !s_rc ) {
        return false;
    }

    s_rc->display();
    return true;
}

static int
SaveImage(ImageOutputHandle *ip) {
    if ( !s_rc ) {
        return false;
    }

    s_rc->save(ip);
    return true;
}

static void
Interrupt() {
    if ( s_rc ) {
        s_rc->interrupt();
    }
}

static void
Terminate() {
    if ( s_rc ) {
        delete s_rc;
    }
    s_rc = 0;
}

static void
Defaults() {
}

static void
ParseHWRCastOptions(int *argc, char **argv) {
}

static void
Initialize() {
}

static void
IRayCast(ImageOutputHandle *ip) {
    if ( s_rc ) {
        delete s_rc;
    }
    s_rc = new RayCaster();
    s_rc->render();
    if ( ip ) {
        s_rc->save(ip);
    }
}

/**
Ray-Casts the current Radiance solution. Output is displayed on the sceen
and saved into the file with given name and file pointer. 'ispipe'
reflects whether this file pointer is a pipe or not.
*/
void
rayCast(char *fname, FILE *fp, int ispipe) {
    ImageOutputHandle *img = nullptr;

    if ( fp ) {
        img = createRadianceImageOutputHandle(fname, fp, ispipe,
                                              GLOBAL_camera_mainCamera.xSize, GLOBAL_camera_mainCamera.ySize,
                                              GLOBAL_statistics_referenceLuminance / 179.0);
        if ( !img ) {
            return;
        }
    }

    RayCaster *rc = new RayCaster;
    rc->render();
    if ( img ) {
        rc->save(img);
    }
    delete rc;

    if ( img ) {
        deleteImageOutputHandle(img);
    }
}

Raytracer GLOBAL_rayCasting_RayCasting = {
        "RayCasting",
        4,
        "Ray Casting",
        Defaults,
        ParseHWRCastOptions,
        Initialize,
        IRayCast,
        Redisplay,
        SaveImage,
        Interrupt,
        Terminate
};
