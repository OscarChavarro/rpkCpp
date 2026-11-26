#ifndef __SCREEN_BUFFER__
#define __SCREEN_BUFFER__

#include "java/io/OutputStream.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector2Dd.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/scene/Camera.h"
#include "vsdk/toolkit/io/image/ImageOutputHandle.h"
#include "vsdk/toolkit/tonemap/ToneMappingContext.h"

/**
Class for storing pixel radiance/fluxes
and an associated RGB framebuffer

12/10/99 : The screen buffer has now a camera variable that states
           from where the image is made. Several functions
           are provided to handle pixel double coordinates,
           pixel numbers and screen boundaries. These are
           derived from the camera variable
*/

class ScreenBuffer {
  private:
    double *radiance;
    ColorRgb *rgbColor;
    Camera camera;

    bool synced;
    double factor;
    double addFactor;
    bool rgbImage; // Indicates an RGB image ( = no radiance conversion!)
    ToneMappingContext *toneMapOptions;

    void init(const Camera *inCamera, const Camera *defaultCamera);

  protected:
    void syncLine(int lineNumber);
    const ToneMappingContext &requireToneMappingContext() const;

  public:
    explicit ScreenBuffer(const Camera *camera, const Camera *defaultCamera, ToneMappingContext *toneMapOptions = nullptr); // Also calls mainInitApplication()
    ~ScreenBuffer();
    bool isRgbImage() const;
    void copy(const ScreenBuffer *source, const Camera *defaultCamera);
    void merge(const ScreenBuffer *src1, const ScreenBuffer *src2, const Camera *defaultCamera);
    double getScreenXMin() const;
    double getScreenYMin() const;
    double getPixXSize() const;
    double getPixYSize() const;
    Vector2Dd getPixelPoint(int nx, int ny, double xOffset = 0.5, double yOffset = 0.5) const;
    Vector2Dd getPixelCenter(int nx, int ny) const;
    int getNx(double x) const;
    int getNy(double y) const;
    void getPixel(double x, double y, int *nx, int *ny) const;
    Vector3Dd getPixelVector(int nx, int ny, double xOffset = 0.5, double yOffset = 0.5) const;
    int getHRes() const;
    int getVRes() const;
    ColorRgb get(int x, int y) const;
    void set(int x, int y, const ColorRgb &inRadiance);
    void render();
    void renderScanline(int y);
    void writeFile(ImageOutputHandle *ip);
    void add(int x, int y, const ColorRgb &inRadiance);
    void sync();
    void setToneMappingContext(ToneMappingContext *inToneMapOptions);

#ifdef RAYTRACING_ENABLED
    double getScreenXMax() const;
    double getScreenYMax() const;
    static double computeFluxToRadFactor(const Camera *camera, int pixX, int pixY);
    ColorRgb getBiLinear(double x, double y) const;
    void scaleRadiance(double inFactor);
    void setAddScaleFactor(double inFactor);
    void setFactor(double inFactor);
    void setRgbImage(bool isRGB);
    void writeFile(const char *fileName, java::OutputStream *outputStream, int isPipe);
    void writeFile(const char *fileName);
#endif

};

#endif
