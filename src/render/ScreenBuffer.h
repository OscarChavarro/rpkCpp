#ifndef __SCREEN_BUFFER__
#define __SCREEN_BUFFER__

#include <cmath>
#include <cstdio>

#include "common/linealAlgebra/Vector2D.h"
#include "IMAGE/imagecpp.h"
#include "common/color.h"
#include "scene/Camera.h"

/**
Class for storing pixel radiance/fluxes
and an associated RGB framebuffer

12/10/99 : The screen buffer has now a GLOBAL_camera_mainCamera variable that states
           from where the image is made. Several functions
           are provided to handle pixel float coordinates,
           pixel numbers and screen boundaries. These are
           derived from the camera variable
*/

class ScreenBuffer {
  private:
    COLOR *m_Radiance;
    RGB *m_RGB;
    Camera m_cam; /* GLOBAL_camera_mainCamera used (copied, no pointer!) */

    bool m_Synced;
    float m_Factor;
    float m_AddFactor;
    bool m_RGBImage; /* Indicates an RGB image (=no radiance conversion!) */

  public:
    explicit ScreenBuffer(Camera *cam); /* Also calls mainInit() */
    ~ScreenBuffer();
    void init(Camera *cam = nullptr);
    void setRgbImage(bool isRGB);
    bool isRgbImage() const;
    void copy(ScreenBuffer *source);
    void merge(ScreenBuffer *src1, ScreenBuffer *src2);
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
    COLOR get(int x, int y);
    void set(int x, int y, COLOR radiance);
    COLOR getBiLinear(float x, float y);
    void render();
    void renderScanline(int line);
    void writeFile(ImageOutputHandle *ip);
    void writeFile(char *filename);
    void add(int x, int y, COLOR radiance);
    void setFactor(float factor);
    void setAddScaleFactor(float factor);
    void scaleRadiance(float factor);
    void sync();

  protected:
    void syncLine(int lineNumber);
};

extern float computeFluxToRadFactor(int pixX, int pixY);

#endif
