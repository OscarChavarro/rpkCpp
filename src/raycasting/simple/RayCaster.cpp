/**
Ray casting using the SGL library for rendering Patch pointers into
a software frame buffer directly.
*/

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED
    #include <ctime>
#endif

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "IMAGE/imagec.h"
#include "common/Statistics.h"
#include "render/SoftIdsWrapper.h"
#include "raycasting/simple/RayCaster.h"

char RayCaster::name[12] = "Ray Casting";

static RayCaster *globalRayCaster = nullptr;

RayCaster::RayCaster(ScreenBuffer *inScreen, const Camera *defaultCamera) {
    if ( inScreen == nullptr ) {
        screenBuffer = new ScreenBuffer(nullptr, defaultCamera);
        doDeleteScreen = true;
    } else {
        screenBuffer = inScreen;
        doDeleteScreen = true;
    }
}

RayCaster::~RayCaster() {
    if ( doDeleteScreen ) {
        delete screenBuffer;
    }
}

void
RayCaster::defaults() {
}

const char *
RayCaster::getName() const {
    return name;
}

void
RayCaster::initialize(const java::ArrayList<Patch *> *lightPatches) const {
}

void
RayCaster::execute(
    ImageOutputHandle *ip,
    Scene *scene,
    RadianceMethod *radianceMethod,
    const RenderOptions *renderOptions) const
{
    if ( globalRayCaster != nullptr ) {
        delete globalRayCaster;
    }
    globalRayCaster = new RayCaster(nullptr, scene->camera);
    globalRayCaster->render(scene, radianceMethod, renderOptions);
    if ( globalRayCaster != nullptr && ip != nullptr ) {
        globalRayCaster->save(ip);
    }
}

/**
Returns false if there is no previous image and true if there is
*/
bool
RayCaster::reDisplay() const {
    if ( !globalRayCaster ) {
        return false;
    }

    globalRayCaster->display();
    return true;
}

bool
RayCaster::saveImage(ImageOutputHandle *imageOutputHandle) const {
    if ( !globalRayCaster ) {
        return false;
    }

    globalRayCaster->save(imageOutputHandle);
    return true;
}

void
RayCaster::terminate() const {
    if ( globalRayCaster ) {
        delete globalRayCaster;
    }
    globalRayCaster = nullptr;
}

void
RayCaster::clipUv(int numberOfVertices, double *u, double *v) {
    if ( *u > 1.0 - Numeric::Numeric::EPSILON ) {
        *u = 1.0 - Numeric::EPSILON;
    }
    if ( *v > 1.0 - Numeric::EPSILON ) {
        *v = 1.0 - Numeric::EPSILON;
    }
    if ( numberOfVertices == 3 && (*u + *v) > 1.0 - Numeric::EPSILON ) {
        if ( *u > *v ) {
            *u = 1.0 - *v - Numeric::EPSILON;
        } else {
            *v = 1.0 - *u - Numeric::EPSILON;
        }
    }
    if ( *u < Numeric::EPSILON ) {
        *u = Numeric::EPSILON;
    }
    if ( *v < Numeric::EPSILON ) {
        *v = Numeric::EPSILON;
    }
}

/**
Determines the radiance of the nearest patch visible through the pixel
(x,y). P shall be the nearest patch visible in the pixel.
*/
inline ColorRgb
RayCaster::getRadianceAtPixel(
    Camera *camera,
    int x,
    int y,
    Patch *patch,
    const RadianceMethod *radianceMethod,
    const RenderOptions *renderOptions) const
{
    ColorRgb radiance{};
    radiance.clear();

    if ( radianceMethod != nullptr ) {
        // Ray pointing from the eye through the center of the pixel.
        Ray ray;
        ray.pos = camera->eyePosition;
        ray.dir = screenBuffer->getPixelVector(x, y);
        ray.dir.normalize(Numeric::EPSILON_FLOAT);

        // Find intersection point of ray with patch
        Vector3D point;
        float dist = patch->normal.dotProduct(ray.dir);
        dist = -(patch->normal.dotProduct(ray.pos) + patch->planeConstant) / dist;
        point.sumScaled(ray.pos, dist, ray.dir);

        // Find surface coordinates of hit point on patch
        double u;
        double v;
        patch->uv(&point, &u, &v);

        // Boundary check is necessary because Z-buffer algorithm does
        // not yield exactly the same result as ray tracing at patch
        // boundaries.
        clipUv(patch->numberOfVertices, &u, &v);

        // Reverse ray direction and get radiance emitted at hit point towards the eye
        Vector3D dir(-ray.dir.x, -ray.dir.y, -ray.dir.z);
        radiance = radianceMethod->getRadiance(camera, patch, u, v, dir, renderOptions);
    }
    return radiance;
}

void
RayCaster::render(
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    const RenderOptions *renderOptions)
{
#ifdef RAYTRACING_ENABLED
    clock_t t = clock();
#endif

    SoftIdsWrapper *idRenderer = new SoftIdsWrapper(scene, renderOptions);

    long width;
    long height;
    idRenderer->getSize(&width, &height);
    if ( width != screenBuffer->getHRes() || height != screenBuffer->getVRes() ) {
        logFatal(-1, "RayCaster::render", "ID buffer size doesn't match screen size");
    }

    // This is the main loop for ray-casting
    for ( int y = 0; y < height; y++ ) {
        for ( int x = 0; x < width; x++ ) {
            Patch *patch = idRenderer->getPatchAtPixel(x, y);
            if ( patch != nullptr ) {
                ColorRgb rad = getRadianceAtPixel(scene->camera, x, y, patch, radianceMethod, renderOptions);
                screenBuffer->add(x, y, rad);
            }
        }

        screenBuffer->renderScanline(y);
    }

    delete idRenderer;

#ifdef RAYTRACING_ENABLED
    GLOBAL_raytracer_totalTime = (float) (clock() - t) / (float) CLOCKS_PER_SEC;
    GLOBAL_raytracer_rayCount = 0;
    GLOBAL_raytracer_pixelCount = 0;
#endif
}

void
RayCaster::display() {
    screenBuffer->render();
}

void
RayCaster::save(ImageOutputHandle *ip) {
    screenBuffer->writeFile(ip);
}

/**
Ray-Casts the current Radiance solution. Output is displayed on the screen
and saved into the file with given name and file pointer. 'isPipe'
reflects whether this file pointer is a pipe or not.
*/
void
rayCast(
    const char *fileName,
    FILE *fp,
    const int isPipe,
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    const RenderOptions *renderOptions) {
    ImageOutputHandle *img = nullptr;

    if ( fp ) {
        img = createRadianceImageOutputHandle(
            fileName,
            fp,
            isPipe,
            scene->camera->xSize,
            scene->camera->ySize,
            (float)GLOBAL_statistics.referenceLuminance / 179.0f);

        if ( img == nullptr ) {
            return;
        }
    }

    RayCaster *rc = new RayCaster(nullptr, scene->camera);
    rc->render(scene, radianceMethod, renderOptions);
    if ( img != nullptr ) {
        rc->save(img);
    }
    delete rc;

    if ( img ) {
        deleteImageOutputHandle(img);
    }
}
