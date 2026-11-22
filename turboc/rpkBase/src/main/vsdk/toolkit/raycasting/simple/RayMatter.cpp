#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#include <stdlib.h>

#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"

/**
Original version by Vincent Masselus adapted by Pieter Peers (2001-06-01)
*/


#ifdef RAYTRACING_ENABLED
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "java/lang/System.h"
#include "vsdk/toolkit/raycasting/common/Raytools.h"
#include "vsdk/toolkit/raycasting/common/BoxFilter.h"
#include "vsdk/toolkit/raycasting/common/TentFilter.h"
#include "vsdk/toolkit/raycasting/common/NormalFilter.h"
#include "vsdk/toolkit/raycasting/simple/RayMatter.h"

RayMatter *RayMatter::rayMatter = NULL;

RayMatter::RayMatter(
    ScreenBuffer *screen,
    const Camera *camera,
    RayMatterState &inRayMatterState,
    ToneMappingContext *toneMapOptions):
    rayMatterState(inRayMatterState)
{
    if ( screen == NULL ) {
        screenBuffer = new ScreenBuffer(NULL, camera, toneMapOptions);
        doDeleteScreen = false;
    } else {
        screenBuffer = screen;
        screenBuffer->setToneMappingContext(toneMapOptions);
        doDeleteScreen = false;
    }

    pixelFilter = NULL;
    screenBuffer->setRgbImage(true);
}

RayMatter::~RayMatter() {
    if ( doDeleteScreen ) {
        delete screenBuffer;
    }
    if ( pixelFilter != NULL ) {
        delete pixelFilter;
    }
}

void
RayMatter::defaults() {
    // Defaults are owned by the caller-provided RayMatterState instance.
}

const char *
RayMatter::getName() const {
    return RAY_MATTER_NAME;
}

void
RayMatter::initialize(const ArrayList<Patch *> */*lightPatches*/) const {
}

void
RayMatter::execute(
    ImageOutputHandle *ip,
    Scene *scene,
    RadianceMethod */*radianceMethod*/,
    ToneMappingContext *toneMapOptions,
    const RenderOptions * /*renderOptions*/) const
{
    if ( rayMatter != NULL ) {
        delete rayMatter;
    }
    rayMatter = new RayMatter(
        NULL,
        scene->camera,
        rayMatterState,
        toneMapOptions);
    rayMatter->doMatting(scene->camera, scene->getSceneQuery(), scene->getSceneQueryWorkspace());
    if ( ip && rayMatter != NULL ) {
        rayMatter->save(ip);
    }
}

bool
RayMatter::saveImage(ImageOutputHandle *imageOutputHandle) const {
    if ( rayMatter == NULL ) {
        return false;
    }

    rayMatter->save(imageOutputHandle);
    return true;
}

void
RayMatter::terminate() const {
    if ( rayMatter != NULL ) {
        delete rayMatter;
    }
    rayMatter = NULL;
}

void
RayMatter::createFilter() {
    if ( pixelFilter != NULL ) {
        delete pixelFilter;
        pixelFilter = NULL;
    }

    if ( rayMatterState.filter == BOX_FILTER ) {
        pixelFilter = new BoxFilter;
    }
    if ( rayMatterState.filter == TENT_FILTER ) {
        pixelFilter = new TentFilter;
    }
    if ( rayMatterState.filter == GAUSS_FILTER ) {
        pixelFilter = new NormalFilter;
    }
    if ( rayMatterState.filter == GAUSS2_FILTER ) {
        pixelFilter = new NormalFilter(0.5, 1.5);
    }
}

void
RayMatter::doMatting(const Camera *camera, const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneWorldSceneQueryWorkspace) {
    const long t = System::nanoTime();

    createFilter();

    // Main loop for ray matter
    for ( int y = 0; y < camera->ySize; y++ ) {
        for ( int x = 0; x < camera->xSize; x++ ) {
            float hits = 0;

            for ( int i = 0; i < rayMatterState.samplesPerPixel; i++ ) {
                // Uniform random var
                double dx = drand48();
                double dy = drand48();

                // Insert non-uniform sampling here
                if ( pixelFilter != NULL ) {
                    pixelFilter->sample(&dx, &dy);
                }

                // Generate ray
                RpkRay ray;
                ray.position = camera->eyePosition;
                ray.direction = screenBuffer->getPixelVector(x, y, ((float)(dx)), ((float)(dy)));
                ray.direction = ray.direction.normalized(Numeric::EPSILON_FLOAT);

                // Check if hit
                if ( RayTools::findRayIntersection(sceneQuery, sceneWorldSceneQueryWorkspace, &ray, NULL, NULL, NULL) != NULL ) {
                    hits++;
                }
            }

            // Add matte value to screen buffer
            float value = (hits / ((float)(rayMatterState.samplesPerPixel)));
            if ( value > 1.0 ) {
                value = 1.0;
            }

            ColorRgb matte;
            matte = ColorRgb(value, value, value);
            screenBuffer->add(x, y, matte);
        }

        screenBuffer->renderScanline(y);
    }

    Statistics::instance().rayTracer.totalTime = ((double)(System::nanoTime() - t)) / 1000000000.0;
    Statistics::instance().rayTracer.rayCount = 0;
    Statistics::instance().rayTracer.pixelCount = 0;
}

void
RayMatter::display() {
    screenBuffer->render();
}

void
RayMatter::save(ImageOutputHandle *ip) {
    screenBuffer->writeFile(ip);
}

#endif
