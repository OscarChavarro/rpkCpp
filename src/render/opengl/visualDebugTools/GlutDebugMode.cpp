#include "render/opengl/visualDebugTools/GlutDebugMode.h"

GlutDebugMode
nextGlutDebugMode(GlutDebugMode mode) {
    switch ( mode ) {
        case GlutDebugMode::RADIANCE_SCENE:
            return GlutDebugMode::GALERKIN_ELEMENT_HIERARCHY;
        case GlutDebugMode::GALERKIN_ELEMENT_HIERARCHY:
            return GlutDebugMode::RADIANCE_SCENE;
        default:
            return GlutDebugMode::RADIANCE_SCENE;
    }
}

const char *
glutDebugModeName(GlutDebugMode mode) {
    switch ( mode ) {
        case GlutDebugMode::RADIANCE_SCENE:
            return "RADIANCE_SCENE";
        case GlutDebugMode::GALERKIN_ELEMENT_HIERARCHY:
            return "GALERKIN_ELEMENT_HIERARCHY";
        default:
            return "UNKNOWN";
    }
}
