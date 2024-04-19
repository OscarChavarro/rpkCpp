#ifndef __SCREEN_BUFFER__
#define __SCREEN_BUFFER__

#include <cmath>
#include <cstdio>

#include "common/linealAlgebra/Vector2D.h"
#include "IMAGE/imagecpp.h"
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
    ColorRgb *m_Radiance;
    ColorRgb *m_RGB;
    Camera camera;

    bool m_Synced;
    float m_Factor;
    float m_AddFactor;
    bool m_RGBImage; // Indicates an RGB image ( = no radiance conversion!)

    void init(Camera *inCamera, Camera *defaultCamera);

  public:
    explicit ScreenBuffer(Camera *camera, Camera *defaultCamera); // Also calls mainInit()
    ~ScreenBuffer();
    void setRgbImage(bool isRGB);
    bool isRgbImage() const;
    void copy(ScreenBuffer *source, Camera *defaultCamera);
    void merge(ScreenBuffer *src1, ScreenBuffer *src2, Camera *defaultCamera);
    float getScreenXMin() const;
    float getScreenYMin() const;
    float getScreenXMax() const;
    float getScreenYMax() const;
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
    ColorRgb get(int x, int y);
    void set(int x, int y, ColorRgb radiance);
    ColorRgb getBiLinear(float x, float y);
    void render();
    void renderScanline(int y);
    void writeFile(ImageOutputHandle *ip);
    void writeFile(char *fileName);
    void add(int x, int y, ColorRgb radiance);
    void setFactor(float factor);
    void setAddScaleFactor(float factor);
    void scaleRadiance(float factor);
    void sync();

  protected:
    void syncLine(int lineNumber);
};

extern float computeFluxToRadFactor(Camera *camera, int pixX, int pixY);

#endif
