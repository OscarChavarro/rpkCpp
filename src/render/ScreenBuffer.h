#ifndef __SCREEN_BUFFER__
#define __SCREEN_BUFFER__

#include <cmath>
#include <cstdio>

#include "common/RenderOptions.h"
#include "common/linealAlgebra/Vector2D.h"
#include "io/image/ImageOutputHandle.h"
#include "common/ColorRgb.h"
#include "scene/Camera.h"

/**
Class for storing pixel radiance/fluxes
and an associated RGB framebuffer

12/10/99 : The screen buffer has now a camera variable that states
           from where the image is made. Several functions
           are provided to handle pixel float coordinates,
           pixel numbers and screen boundaries. These are
           derived from the camera variable
*/

class ScreenBuffer {
  private:
    ColorRgb *radiance;
    ColorRgb *rgbColor;
    Camera camera;

    bool synced;
    float factor;
    float addFactor;
    bool rgbImage; // Indicates an RGB image ( = no radiance conversion!)

    void init(const Camera *inCamera, const Camera *defaultCamera);

  protected:
    void syncLine(int lineNumber);

  public:
    explicit ScreenBuffer(const Camera *camera, const Camera *defaultCamera); // Also calls mainInitApplication()
    ~ScreenBuffer();
    bool isRgbImage() const;
    void copy(const ScreenBuffer *source, const Camera *defaultCamera);
    void merge(const ScreenBuffer *src1, const ScreenBuffer *src2, const Camera *defaultCamera);
    float getScreenXMin() const;
    float getScreenYMin() const;
    float getPixXSize() const;
    float getPixYSize() const;
    Vector2D getPixelPoint(int nx, int ny, float xOffset = 0.5, float yOffset = 0.5) const;
    Vector2D getPixelCenter(int nx, int ny) const;
    int getNx(float x) const;
    int getNy(float y) const;
    void getPixel(float x, float y, int *nx, int *ny) const;
    Vector3D getPixelVector(int nx, int ny, float xOffset = 0.5, float yOffset = 0.5) const;
    int getHRes() const;
    int getVRes() const;
    ColorRgb get(int x, int y) const;
    void set(int x, int y, ColorRgb inRadiance);
    void render();
    void renderScanline(int y);
    void writeFile(ImageOutputHandle *ip);
    void add(int x, int y, ColorRgb inRadiance);
    void sync();

#ifdef RAYTRACING_ENABLED
    float getScreenXMax() const;
    float getScreenYMax() const;
    ColorRgb getBiLinear(float x, float y) const;
    void scaleRadiance(float inFactor);
    void setAddScaleFactor(float inFactor);
    void setFactor(float inFactor);
    void setRgbImage(bool isRGB);
    void writeFile(const char *fileName);
#endif

};

#ifdef RAYTRACING_ENABLED
    extern float computeFluxToRadFactor(const Camera *camera, int pixX, int pixY);
#endif

#endif
