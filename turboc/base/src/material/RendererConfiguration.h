#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __RENDER_OPTIONS__
#define __RENDER_OPTIONS__

#include "common/color/ColorRgb.h"

/**
If this is undefined, the raytracing code can be trimmed as follows:
- PHOTON MAP module can be removed
- All of the ray-casting module can be removed except the RayCaster class

Build-time feature flags are configured from CMake.
*/

class RenderOptions {
  private:
    ColorRgb outlineColor; // Color in which to draw outlines
    ColorRgb boundingBoxColor; // Color in which to draw bounding boxes
    ColorRgb clusterColor; // Color in which to show cluster bounding boxes
    float lineWidth;
    bool drawOutlines; // True for drawing facet outlines
    bool drawSurfaces;
    char noShading; // False for using any kind of shading
    char smoothShading;  // True for rendering with Gouraud interpolation
    char backfaceCulling; // True for backface culling
    bool drawBoundingBoxes; // True for showing bounding boxes
    bool drawClusters; // True for showing cluster hierarchy
    char frustumCulling; // Frustum culling accelerates rendering of large scenes.
    char renderRayTracedImage; // For freezing ray-traced image on the screen when appropriate
    char trace; // High-dynamic range ray-traced tiff

    #define DEFAULT_SMOOTH_SHADING true
    #define DEFAULT_BACKFACE_CULLING true
    #define DEFAULT_OUTLINE_DRAWING false
    #define DEFAULT_SURFACE_DRAWING true
    #define DEFAULT_BOUNDING_BOX_DRAWING false
    #define DEFAULT_CLUSTER_DRAWING false
    static const ColorRgb DEFAULT_OUTLINE_COLOR;
    static const ColorRgb DEFAULT_BOUNDING_BOX_COLOR;
    static const ColorRgb DEFAULT_CLUSTER_COLOR;

  public:
    RenderOptions();
    virtual ~RenderOptions();

    float getLineWidth() const;
    bool isDrawOutlines() const;
    bool isDrawSurfaces() const;
    char isNoShading() const;
    char isSmoothShading() const;
    char isBackfaceCulling() const;
    bool isDrawBoundingBoxes() const;
    bool isDrawClusters() const;
    char isFrustumCulling() const;
    char isRenderRayTracedImage() const;
    char isTrace() const;

    void setLineWidth(float width);
    void setDrawOutlines(bool enabled);
    void setDrawSurfaces(bool enabled);
    void setNoShading(char enabled);
    void setSmoothShading(char enabled);
    void setBackfaceCulling(char enabled);
    void setDrawBoundingBoxes(bool enabled);
    void setDrawClusters(bool enabled);
    void setFrustumCulling(char enabled);
    void setRenderRayTracedImage(char enabled);
    void setTrace(char enabled);

    const ColorRgb &getOutlineColor() const;
    const ColorRgb &getBoundingBoxColor() const;
    const ColorRgb &getClusterColor() const;
    void setOutlineColor(const ColorRgb &color);
    void setBoundingBoxColor(const ColorRgb &color);
    void setClusterColor(const ColorRgb &color);
};

inline const ColorRgb &
RenderOptions::getOutlineColor() const { return outlineColor; }
inline const ColorRgb &
RenderOptions::getBoundingBoxColor() const { return boundingBoxColor; }
inline const ColorRgb &
RenderOptions::getClusterColor() const { return clusterColor; }
inline float
RenderOptions::getLineWidth() const { return lineWidth; }
inline bool
RenderOptions::isDrawOutlines() const { return drawOutlines; }
inline bool
RenderOptions::isDrawSurfaces() const { return drawSurfaces; }
inline char
RenderOptions::isNoShading() const { return noShading; }
inline char
RenderOptions::isSmoothShading() const { return smoothShading; }
inline char
RenderOptions::isBackfaceCulling() const { return backfaceCulling; }
inline bool
RenderOptions::isDrawBoundingBoxes() const { return drawBoundingBoxes; }
inline bool
RenderOptions::isDrawClusters() const { return drawClusters; }
inline char
RenderOptions::isFrustumCulling() const { return frustumCulling; }
inline char
RenderOptions::isRenderRayTracedImage() const { return renderRayTracedImage; }
inline char
RenderOptions::isTrace() const { return trace; }

#endif
