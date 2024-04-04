#include <cstring>

#include "common/error.h"
#include "material/statistics.h"
#include "render/opengl.h"
#include "io/FileUncompressWrapper.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "render/ScreenBuffer.h"

RGB GLOBAL_material_black = {0.0, 0.0, 0.0};
RGB GLOBAL_material_yellow = {1.0, 1.0, 0.0};
RGB GLOBAL_material_white = {1.0, 1.0, 1.0};

/**
Constructor : make an screen buffer from a camera definition
If (cam == nullptr) the current camera (GLOBAL_camera_mainCamera) is taken
*/
ScreenBuffer::ScreenBuffer(Camera *cam) {
    m_Radiance = nullptr;
    m_RGB = nullptr;
    init(cam);
    m_Synced = false;
    m_Factor = 1.0;
    m_AddFactor = 1.0;
    m_RGBImage = false;
}

ScreenBuffer::~ScreenBuffer() {
    if ( m_Radiance != nullptr ) {
        free((char *)m_Radiance);
        m_Radiance = nullptr;
    }

    if ( m_RGB != nullptr ) {
        free((char *)m_RGB);
        m_RGB = nullptr;
    }
}

void
ScreenBuffer::setRgbImage(bool isRGB) {
    m_RGBImage = isRGB;
}

bool
ScreenBuffer::isRgbImage() const {
    return m_RGBImage;
}

void
ScreenBuffer::init(Camera *cam) {
    int i;

    if ( cam == nullptr ) {
        cam = &GLOBAL_camera_mainCamera;
    } // Use the current camera

    if ( (m_Radiance != nullptr) &&
        ((cam->xSize != m_cam.xSize) ||
         (cam->ySize != m_cam.ySize)) ) {
        free((char *) m_RGB);
        free((char *) m_Radiance);
        m_Radiance = nullptr;
    }

    m_cam = *cam;

    if ( m_Radiance == nullptr ) {
        m_Radiance = (ColorRgb *)malloc(m_cam.xSize * m_cam.ySize *
                                        sizeof(ColorRgb));
        m_RGB = (RGB *)malloc(m_cam.xSize * m_cam.ySize *
                              sizeof(RGB));
    }

    // Clear
    for ( i = 0; i < m_cam.xSize * m_cam.ySize; i++ ) {
        m_Radiance[i].setMonochrome(0.0);
        m_RGB[i] = GLOBAL_material_black;
    }

    m_Factor = 1.0;
    m_AddFactor = 1.0;
    m_Synced = true;
    m_RGBImage = false;
}

/**
Copy dimensions and contents (m_Radiance only) from source
*/
void
ScreenBuffer::copy(ScreenBuffer *source) {
    init(&(source->m_cam));
    m_RGBImage = source->isRgbImage();

    // Now the resolution is ok.

    memcpy(m_Radiance, source->m_Radiance, m_cam.xSize * m_cam.ySize * sizeof(ColorRgb));
    m_Synced = false;
}

/**
Merge (add) two screen buffers (m_Radiance only) from src1 and src2
*/
void
ScreenBuffer::merge(ScreenBuffer *src1, ScreenBuffer *src2) {
    init(&(src1->m_cam));
    m_RGBImage = src1->isRgbImage();

    if ( (getHRes() != src2->getHRes()) || (getVRes() != src2->getVRes()) ) {
        logError("ScreenBuffer::merge", "Incompatible screen buffer sources");
        return;
    }

    int N = getVRes() * getHRes();

    for ( int i = 0; i < N; i++ ) {
        colorAdd(src1->m_Radiance[i], src2->m_Radiance[i], m_Radiance[i]);
    }
}

void
ScreenBuffer::add(int x, int y, ColorRgb radiance) {
    int index = x + (m_cam.ySize - y - 1) * m_cam.xSize;

    colorAddScaled(m_Radiance[index], m_AddFactor, radiance,
                   m_Radiance[index]);
    m_Synced = false;
}

void
ScreenBuffer::set(int x, int y, ColorRgb radiance) {
    int index = x + (m_cam.ySize - y - 1) * m_cam.xSize;
    m_Radiance[index].scaledCopy(m_AddFactor, radiance);
    m_Synced = false;
}

void
ScreenBuffer::scaleRadiance(float factor) {
    for ( int i = 0; i < m_cam.xSize * m_cam.ySize; i++ ) {
        m_Radiance[i].scale(factor);
    }

    m_Synced = false;
}

ColorRgb
ScreenBuffer::get(int x, int y) {
    int index = x + (m_cam.ySize - y - 1) * m_cam.xSize;

    return m_Radiance[index];
}

ColorRgb
ScreenBuffer::getBiLinear(float x, float y) {
    int nx0, nx1, ny0, ny1;
    Vector2D center;
    ColorRgb col{};

    getPixel(x, y, &nx0, &ny0);
    center = getPixelCenter(nx0, ny0);

    x = (x - center.u) / getPixXSize();
    y = (y - center.v) / getPixYSize();

    if ( x < 0 ) {
        // Point on left side of pixel center
        x = -x;
        nx1 = intMax(nx0 - 1, 0);
    } else {
        nx1 = intMin(getHRes(), nx0 + 1);
    }

    if ( y < 0 ) {
        y = -y;
        ny1 = intMax(ny0 - 1, 0);
    } else {
        ny1 = intMin(getVRes(), ny0 + 1);
    }

    // u = 0 for nx0 and u = 1 for nx1, x in-between. Not that
    // nx0 and nx1 may be the same (at border of image). Same for ny

    ColorRgb c0 = get(nx0, ny0); // Separate vars, since interpolation is a macro...
    ColorRgb c1 = get(nx1, ny0); // u = 1
    ColorRgb c2 = get(nx1, ny1); // u = 1, v = 1
    ColorRgb c3 = get(nx0, ny1); // v = 1

    colorInterpolateBiLinear(c0, c1, c2, c3, x, y, col);

    return col;
}

void
ScreenBuffer::setFactor(float factor) {
    m_Factor = factor;
}

void
ScreenBuffer::setAddScaleFactor(float factor) {
    m_AddFactor = factor;
}

void
ScreenBuffer::render() {
    if ( !m_Synced ) {
        sync();
    }

    openGlRenderPixels(0, 0, m_cam.xSize, m_cam.ySize, m_RGB);
}

void
ScreenBuffer::writeFile(ImageOutputHandle *ip) {
    if ( !ip ) {
        return;
    }

    if ( !m_Synced ) {
        sync();
    }

    fprintf(stderr, "Writing %s file ... ", ip->driverName);

    ip->gamma[0] = GLOBAL_toneMap_options.gamma.r; // For default radiance -> display RGB
    ip->gamma[1] = GLOBAL_toneMap_options.gamma.g;
    ip->gamma[2] = GLOBAL_toneMap_options.gamma.b;
    for ( int i = m_cam.ySize - 1; i >= 0; i-- ) {
        // Write scan lines
        if ( !isRgbImage() ) {
            ip->writeRadianceRGB((float *) &m_Radiance[i * m_cam.xSize]);
        } else {
            ip->writeDisplayRGB((float *) &m_Radiance[i * m_cam.xSize]);
        }
    }

    fprintf(stderr, "done.\n");
}

void
ScreenBuffer::writeFile(char *filename) {
    int isPipe;
    FILE *fp = openFileCompressWrapper(filename, "w", &isPipe);
    if ( !fp ) {
        return;
    }

    ImageOutputHandle *ip =
            createRadianceImageOutputHandle(filename, fp, isPipe,
                                            m_cam.xSize, m_cam.ySize,
                                            (float) GLOBAL_statistics.referenceLuminance / 179.0f);

    writeFile(ip);

    // DeleteImageOutputHandle(ip);
    closeFile(fp, isPipe);
}

void
ScreenBuffer::renderScanline(int i) {
    i = m_cam.ySize - i - 1;

    if ( !m_Synced ) {
        syncLine(i);
    }

    openGlRenderPixels(0, i, m_cam.xSize, 1, &m_RGB[i * m_cam.xSize]);
}

void
ScreenBuffer::sync() {
    int i;
    ColorRgb tmpRad{};

    for ( i = 0; i < m_cam.xSize * m_cam.ySize; i++ ) {
        tmpRad.scaledCopy(m_Factor, m_Radiance[i]);
        if ( !isRgbImage() ) {
            radianceToRgb(tmpRad, &m_RGB[i]);
        } else {
            convertColorToRGB(tmpRad, &m_RGB[i]);
        }
    }

    m_Synced = true;
}


void
ScreenBuffer::syncLine(int lineNumber) {
    int i;
    ColorRgb tmpRad{};

    for ( i = 0; i < m_cam.xSize; i++ ) {
        tmpRad.scaledCopy(m_Factor, m_Radiance[lineNumber * m_cam.xSize + i]);
        if ( !isRgbImage() ) {
            radianceToRgb(tmpRad, &m_RGB[lineNumber * m_cam.xSize + i]);
        } else {
            convertColorToRGB(tmpRad, &m_RGB[lineNumber * m_cam.xSize + i]);
        }
    }
}

float
ScreenBuffer::getScreenXMin() const {
    return -m_cam.pixelWidth * (float)m_cam.xSize / 2.0f;
}

float
ScreenBuffer::getScreenYMin() const {
    return -m_cam.pixelHeight * (float)m_cam.ySize / 2.0f;
}

float
ScreenBuffer::getScreenXMax() const {
    return m_cam.pixelWidth * (float)m_cam.xSize / 2.0f;
}

float
ScreenBuffer::getScreenYMax() const {
    return m_cam.pixelHeight * (float)m_cam.ySize / 2.0f;
}

float
ScreenBuffer::getPixXSize() const {
    return m_cam.pixelWidth;
}

float
ScreenBuffer::getPixYSize() const {
    return m_cam.pixelHeight;
}

Vector2D
ScreenBuffer::getPixelPoint(int nx, int ny, float xOffset, float yOffset) const {
    return {getScreenXMin() + ((float) nx + xOffset) * getPixXSize(),
            getScreenYMin() + ((float) ny + yOffset) * getPixYSize()};
}

Vector2D
ScreenBuffer::getPixelCenter(int nx, int ny) const {
    return getPixelPoint(nx, ny, 0.5, 0.5);
}

int
ScreenBuffer::getNx(float x) const {
    return (int) std::floor((x - getScreenXMin()) / getPixXSize());
}

int
ScreenBuffer::getNy(float y) const {
    return (int) std::floor((y - getScreenYMin()) / getPixYSize());
}

void
ScreenBuffer::getPixel(float x, float y, int *nx, int *ny) const {
    *nx = getNx(x);
    *ny = getNy(y);
}

/**
Un-normalized vector pointing from the eye point to the
point with given fractional pixel coordinates
*/
Vector3D
ScreenBuffer::getPixelVector(int nx, int ny, float xOffset, float yOffset) const {
    Vector2D pix = getPixelPoint(nx, ny, xOffset, yOffset);
    Vector3D dir;
    vectorComb3(m_cam.Z, pix.u, m_cam.X, pix.v, m_cam.Y, dir);
    return dir;
}

/**
Screen resolution
*/
int
ScreenBuffer::getHRes() const {
    return m_cam.xSize;
}

int
ScreenBuffer::getVRes() const {
    return m_cam.ySize;
}

float
computeFluxToRadFactor(int pixX, int pixY) {
    Vector3D dir;
    double h = GLOBAL_camera_mainCamera.pixelWidth;
    double v = GLOBAL_camera_mainCamera.pixelHeight;

    double x = -h * GLOBAL_camera_mainCamera.xSize / 2.0 + pixX * h;
    double y = -v * GLOBAL_camera_mainCamera.ySize / 2.0 + pixY * v;

    double xSample = x + h * 0.5;  // (pixX, pixY) indicate upper left
    double ySample = y + v * 0.5;

    vectorComb3(GLOBAL_camera_mainCamera.Z, (float)xSample, GLOBAL_camera_mainCamera.X, (float)ySample, GLOBAL_camera_mainCamera.Y,
                dir);
    double distPixel2 = vectorNorm2(dir);
    double distPixel = std::sqrt(distPixel2);
    vectorScaleInverse((float)distPixel, dir, dir);

    double factor = 1.0 / (h * v);

    factor *= distPixel2; // r(eye->pixel)^2
    factor /= std::pow(vectorDotProduct(dir, GLOBAL_camera_mainCamera.Z), 2);  // cos^2

    return (float)factor;
}
