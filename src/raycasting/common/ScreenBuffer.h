#ifndef __SCREENBUFFER__
#define __SCREENBUFFER__

#include <cmath>
#include <cstdio>

#include "IMAGE/imagecpp.h"
#include "material/color.h"
#include "shared/Camera.h"

/*************************************************************************/
/* Class for storing pixel radiances/fluxes                              */
/* and an associated RGB framebuffer                                     */
/*                                                                       */
/* 12/10/99 : The screen buffer has now a GLOBAL_camera_mainCamera variable that states    */
/*            from where the image is made. Several functions            */
/*            are provided to handle pixel float coordinates,            */
/*            pixel numbers and screen boundaries. These are             */
/*            derived from the camera variable                           */
/*************************************************************************/

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
    void Init(Camera *cam = nullptr);
    void SetRGBImage(bool isRGB);
    bool IsRGBImage() const;
    void Copy(ScreenBuffer *source);
    void Merge(ScreenBuffer *src1, ScreenBuffer *src2);
    float GetScreenXMin() const;
    float GetScreenYMin() const;
    float GetScreenXMax() const;
    float GetScreenYMax() const;
    float GetPixXSize() const;
    float GetPixYSize() const;
    Vector2D GetPixelPoint(int nx, int ny, float xoff = 0.5, float yoff = 0.5) const;
    Vector2D GetPixelCenter(int nx, int ny) const;
    int GetNx(float x) const;
    int GetNy(float y) const;
    void GetPixel(float x, float y, int *nx, int *ny) const;
    Vector3D GetPixelVector(int nx, int ny, float xoff = 0.5, float yoff = 0.5) const;
    int GetHRes() const;
    int GetVRes() const;
    COLOR Get(int x, int y);
    void Set(int x, int y, COLOR radiance);
    COLOR GetBiLinear(float x, float y);
    void Render();
    void RenderScanline(int line);
    void WriteFile(ImageOutputHandle *ip);
    void WriteFile(char *filename);
    void Add(int x, int y, COLOR radiance);
    void SetFactor(float factor);
    void SetAddScaleFactor(float factor);
    void ScaleRadiance(float factor);
    void Sync();

  protected:
    void SyncLine(int lineNumber);
};

extern float ComputeFluxToRadFactor(int pix_x, int pix_y);

#endif
