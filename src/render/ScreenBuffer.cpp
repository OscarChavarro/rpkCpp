#include <cstring>

#include "java/lang/Math.h"
#include "common/error.h"
#include "common/Statistics.h"
#include "render/opengl.h"
#include "io/FileUncompressWrapper.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "render/ScreenBuffer.h"

/**
Constructor : make an screen buffer from a camera definition
*/
ScreenBuffer::ScreenBuffer(Camera *camera, Camera *defaultCamera) {
    m_Radiance = nullptr;
    m_RGB = nullptr;
    init(camera, defaultCamera);
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

bool
ScreenBuffer::isRgbImage() const {
    return m_RGBImage;
}

void
ScreenBuffer::init(Camera *inCamera, Camera *defaultCamera) {
    if ( inCamera == nullptr ) {
        // Use the current camera
        inCamera = defaultCamera;
    }

    if ( (m_Radiance != nullptr) &&
        ((inCamera->xSize != camera.xSize) || (inCamera->ySize != camera.ySize)) ) {
        free((char *) m_RGB);
        free((char *) m_Radiance);
        m_Radiance = nullptr;
    }

    camera = *inCamera;

    if ( m_Radiance == nullptr ) {
        m_Radiance = (ColorRgb *)malloc(camera.xSize * camera.ySize * sizeof(ColorRgb));
        m_RGB = (ColorRgb *)malloc(camera.xSize * camera.ySize * sizeof(ColorRgb));
    }

    // Clear
    ColorRgb black = {0.0, 0.0, 0.0};
    for ( int i = 0; i < camera.xSize * camera.ySize; i++ ) {
        m_Radiance[i].setMonochrome(0.0);
        m_RGB[i] = black;
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
ScreenBuffer::copy(ScreenBuffer *source, Camera *defaultCamera) {
    init(&(source->camera), defaultCamera);
    m_RGBImage = source->isRgbImage();

    // Now the resolution is ok.

    memcpy(m_Radiance, source->m_Radiance, camera.xSize * camera.ySize * sizeof(ColorRgb));
    m_Synced = false;
}

/**
Merge (add) two screen buffers (m_Radiance only) from src1 and src2
*/
void
ScreenBuffer::merge(ScreenBuffer *src1, ScreenBuffer *src2, Camera *defaultCamera) {
    init(&(src1->camera), defaultCamera);
    m_RGBImage = src1->isRgbImage();

    if ( (getHRes() != src2->getHRes()) || (getVRes() != src2->getVRes()) ) {
        logError("ScreenBuffer::merge", "Incompatible screen buffer sources");
        return;
    }

    int N = getVRes() * getHRes();

    for ( int i = 0; i < N; i++ ) {
        m_Radiance[i].add(src1->m_Radiance[i], src2->m_Radiance[i]);
    }
}

void
ScreenBuffer::add(int x, int y, ColorRgb radiance) {
    int index = x + (camera.ySize - y - 1) * camera.xSize;

    m_Radiance[index].addScaled(m_Radiance[index], m_AddFactor, radiance);
    m_Synced = false;
}

void
ScreenBuffer::set(int x, int y, ColorRgb radiance) {
    int index = x + (camera.ySize - y - 1) * camera.xSize;
    m_Radiance[index].scaledCopy(m_AddFactor, radiance);
    m_Synced = false;
}

ColorRgb
ScreenBuffer::get(int x, int y) {
    int index = x + (camera.ySize - y - 1) * camera.xSize;

    return m_Radiance[index];
}

void
ScreenBuffer::render() {
    if ( !m_Synced ) {
        sync();
    }

    openGlRenderPixels(&camera, 0, 0, camera.xSize, camera.ySize, m_RGB);
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
    for ( int i = camera.ySize - 1; i >= 0; i-- ) {
        // Write scan lines
        if ( !isRgbImage() ) {
            ip->writeRadianceRGB((float *) &m_Radiance[i * camera.xSize]);
        } else {
            ip->writeDisplayRGB((float *) &m_Radiance[i * camera.xSize]);
        }
    }

    fprintf(stderr, "done.\n");
}

void
ScreenBuffer::renderScanline(int y) {
    y = camera.ySize - y - 1;

    if ( !m_Synced ) {
        syncLine(y);
    }

    openGlRenderPixels(&camera, 0, y, camera.xSize, 1, &m_RGB[y * camera.xSize]);
}

void
ScreenBuffer::sync() {
    ColorRgb tmpRad{};

    for ( int i = 0; i < camera.xSize * camera.ySize; i++ ) {
        tmpRad.scaledCopy(m_Factor, m_Radiance[i]);
        if ( !isRgbImage() ) {
            radianceToRgb(tmpRad, &m_RGB[i]);
        } else {
            tmpRad.set(m_RGB[i].r, m_RGB[i].g, m_RGB[i].b);
        }
    }

    m_Synced = true;
}


void
ScreenBuffer::syncLine(int lineNumber) {
    int i;
    ColorRgb tmpRad{};

    for ( i = 0; i < camera.xSize; i++ ) {
        tmpRad.scaledCopy(m_Factor, m_Radiance[lineNumber * camera.xSize + i]);
        if ( !isRgbImage() ) {
            radianceToRgb(tmpRad, &m_RGB[lineNumber * camera.xSize + i]);
        } else {
            tmpRad = m_RGB[lineNumber * camera.xSize + i];
        }
    }
}

float
ScreenBuffer::getScreenXMin() const {
    return -camera.pixelWidth * (float)camera.xSize / 2.0f;
}

float
ScreenBuffer::getScreenYMin() const {
    return -camera.pixelHeight * (float)camera.ySize / 2.0f;
}

float
ScreenBuffer::getPixXSize() const {
    return camera.pixelWidth;
}

float
ScreenBuffer::getPixYSize() const {
    return camera.pixelHeight;
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
    dir.combine3(camera.Z, pix.u, camera.X, pix.v, camera.Y);
    return dir;
}

/**
Screen resolution
*/
int
ScreenBuffer::getHRes() const {
    return camera.xSize;
}

int
ScreenBuffer::getVRes() const {
    return camera.ySize;
}

float
computeFluxToRadFactor(const Camera *camera, int pixX, int pixY) {
    Vector3D dir;
    double h = camera->pixelWidth;
    double v = camera->pixelHeight;

    double x = -h * camera->xSize / 2.0 + pixX * h;
    double y = -v * camera->ySize / 2.0 + pixY * v;

    double xSample = x + h * 0.5;  // (pixX, pixY) indicate upper left
    double ySample = y + v * 0.5;

    dir.combine3(camera->Z, (float) xSample, camera->X, (float) ySample, camera->Y);
    double distPixel2 = dir.norm2();
    double distPixel = java::Math::sqrt(distPixel2);
    dir.inverseScaledCopy((float)distPixel, dir, EPSILON_FLOAT);

    double factor = 1.0 / (h * v);

    factor *= distPixel2; // r(eye->pixel)^2
    factor /= std::pow(dir.dotProduct(camera->Z), 2);  // cos^2

    return (float)factor;
}

#ifdef RAYTRACING_ENABLED

float
ScreenBuffer::getScreenXMax() const {
    return camera.pixelWidth * (float)camera.xSize / 2.0f;
}

float
ScreenBuffer::getScreenYMax() const {
    return camera.pixelHeight * (float)camera.ySize / 2.0f;
}

ColorRgb
ScreenBuffer::getBiLinear(float x, float y) {
    int nx0;
    int nx1;
    int ny0;
    int ny1;
    Vector2D center;
    ColorRgb color{};

    getPixel(x, y, &nx0, &ny0);
    center = getPixelCenter(nx0, ny0);

    x = (x - center.u) / getPixXSize();
    y = (y - center.v) / getPixYSize();

    if ( x < 0 ) {
        // Point on left side of pixel center
        x = -x;
        nx1 = java::Math::max(nx0 - 1, 0);
    } else {
        nx1 = java::Math::min(getHRes(), nx0 + 1);
    }

    if ( y < 0 ) {
        y = -y;
        ny1 = java::Math::max(ny0 - 1, 0);
    } else {
        ny1 = java::Math::min(getVRes(), ny0 + 1);
    }

    // u = 0 for nx0 and u = 1 for nx1, x in-between. Not that
    // nx0 and nx1 may be the same (at border of image). Same for ny

    ColorRgb c0 = get(nx0, ny0); // Separate vars, since interpolation is a macro...
    ColorRgb c1 = get(nx1, ny0); // u = 1
    ColorRgb c2 = get(nx1, ny1); // u = 1, v = 1
    ColorRgb c3 = get(nx0, ny1); // v = 1

    color.interpolateBiLinear(c0, c1, c2, c3, x, y);

    return color;
}

void
ScreenBuffer::scaleRadiance(float factor) {
    for ( int i = 0; i < camera.xSize * camera.ySize; i++ ) {
        m_Radiance[i].scale(factor);
    }

    m_Synced = false;
}

void
ScreenBuffer::setAddScaleFactor(float factor) {
    m_AddFactor = factor;
}

void
ScreenBuffer::setFactor(float factor) {
    m_Factor = factor;
}

void
ScreenBuffer::setRgbImage(bool isRGB) {
    m_RGBImage = isRGB;
}

void
ScreenBuffer::writeFile(const char *fileName) {
    int isPipe;
    FILE *fp = openFileCompressWrapper(fileName, "w", &isPipe);
    if ( !fp ) {
        return;
    }

    ImageOutputHandle *ip = createRadianceImageOutputHandle(
        fileName,
        fp,
        isPipe,
        camera.xSize,
        camera.ySize,
        (float) GLOBAL_statistics.referenceLuminance / 179.0f);

    writeFile(ip);
    closeFile(fp, isPipe);
}

#endif
