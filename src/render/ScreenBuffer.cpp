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
ScreenBuffer::ScreenBuffer(const Camera *camera, const Camera *defaultCamera) {
    radiance = nullptr;
    rgbColor = nullptr;
    init(camera, defaultCamera);
    synced = false;
    factor = 1.0;
    addFactor = 1.0;
    rgbImage = false;
}

ScreenBuffer::~ScreenBuffer() {
    if ( radiance != nullptr ) {
        delete[] radiance;
        radiance = nullptr;
    }

    if ( rgbColor != nullptr ) {
        delete[] rgbColor;
        rgbColor = nullptr;
    }
}

bool
ScreenBuffer::isRgbImage() const {
    return rgbImage;
}

void
ScreenBuffer::init(const Camera *inCamera, const Camera *defaultCamera) {
    if ( inCamera == nullptr ) {
        // Use the current camera
        inCamera = defaultCamera;
    }

    if ( (radiance != nullptr) && (inCamera->xSize != camera.xSize || inCamera->ySize != camera.ySize) ) {
        delete[] rgbColor;
        delete[] radiance;
        radiance = nullptr;
    }

    camera = *inCamera;

    if ( radiance == nullptr ) {
        radiance = new ColorRgb[camera.xSize * camera.ySize];
        rgbColor = new ColorRgb[camera.xSize * camera.ySize];
    }

    // Clear
    ColorRgb black = {0.0, 0.0, 0.0};
    for ( int i = 0; i < camera.xSize * camera.ySize; i++ ) {
        radiance[i].setMonochrome(0.0);
        rgbColor[i] = black;
    }

    factor = 1.0;
    addFactor = 1.0;
    synced = true;
    rgbImage = false;
}

/**
Copy dimensions and contents (radiance only) from source
*/
void
ScreenBuffer::copy(const ScreenBuffer *source, const Camera *defaultCamera) {
    init(&(source->camera), defaultCamera);
    rgbImage = source->isRgbImage();

    // Now the resolution is ok.

    memcpy(radiance, source->radiance, camera.xSize * camera.ySize * sizeof(ColorRgb));
    synced = false;
}

/**
Merge (add) two screen buffers (radiance only) from src1 and src2
*/
void
ScreenBuffer::merge(const ScreenBuffer *src1, const ScreenBuffer *src2, const Camera *defaultCamera) {
    init(&(src1->camera), defaultCamera);
    rgbImage = src1->isRgbImage();

    if ( (getHRes() != src2->getHRes()) || (getVRes() != src2->getVRes()) ) {
        logError("ScreenBuffer::merge", "Incompatible screen buffer sources");
        return;
    }

    int N = getVRes() * getHRes();

    for ( int i = 0; i < N; i++ ) {
        radiance[i].add(src1->radiance[i], src2->radiance[i]);
    }
}

void
ScreenBuffer::add(int x, int y, ColorRgb inRadiance) {
    int index = x + (camera.ySize - y - 1) * camera.xSize;

    radiance[index].addScaled(radiance[index], addFactor, inRadiance);
    synced = false;
}

void
ScreenBuffer::set(int x, int y, ColorRgb inRadiance) {
    int index = x + (camera.ySize - y - 1) * camera.xSize;
    radiance[index].scaledCopy(addFactor, inRadiance);
    synced = false;
}

ColorRgb
ScreenBuffer::get(int x, int y) const {
    int index = x + (camera.ySize - y - 1) * camera.xSize;

    return radiance[index];
}

void
ScreenBuffer::render() {
    if ( !synced ) {
        sync();
    }

    openGlRenderPixels(&camera, 0, 0, camera.xSize, camera.ySize, rgbColor);
}

void
ScreenBuffer::writeFile(ImageOutputHandle *ip) {
    if ( !ip ) {
        return;
    }

    if ( !synced ) {
        sync();
    }

    fprintf(stderr, "Writing %s file ... ", ip->driverName);

    ip->gamma[0] = GLOBAL_toneMap_options.gamma.r; // For default radiance -> display RGB
    ip->gamma[1] = GLOBAL_toneMap_options.gamma.g;
    ip->gamma[2] = GLOBAL_toneMap_options.gamma.b;
    for ( int i = camera.ySize - 1; i >= 0; i-- ) {
        // Write scan lines
        if ( !isRgbImage() ) {
            ip->writeRadianceRGB((float *) &radiance[i * camera.xSize]);
        } else {
            ip->writeDisplayRGB((float *) &radiance[i * camera.xSize]);
        }
    }

    fprintf(stderr, "done.\n");
}

void
ScreenBuffer::renderScanline(int y) {
    y = camera.ySize - y - 1;

    if ( !synced ) {
        syncLine(y);
    }

    openGlRenderPixels(&camera, 0, y, camera.xSize, 1, &rgbColor[y * camera.xSize]);
}

void
ScreenBuffer::sync() {
    ColorRgb tmpRad{};

    for ( int i = 0; i < camera.xSize * camera.ySize; i++ ) {
        tmpRad.scaledCopy(factor, radiance[i]);
        if ( !isRgbImage() ) {
            radianceToRgb(tmpRad, &rgbColor[i]);
        } else {
            tmpRad.set(rgbColor[i].r, rgbColor[i].g, rgbColor[i].b);
        }
    }

    synced = true;
}


void
ScreenBuffer::syncLine(int lineNumber) {
    ColorRgb tmpRad{};

    for ( int i = 0; i < camera.xSize; i++ ) {
        tmpRad.scaledCopy(factor, radiance[lineNumber * camera.xSize + i]);
        if ( !isRgbImage() ) {
            radianceToRgb(tmpRad, &rgbColor[lineNumber * camera.xSize + i]);
        } else {
            tmpRad = rgbColor[lineNumber * camera.xSize + i];
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
    return (int) java::Math::floor((x - getScreenXMin()) / getPixXSize());
}

int
ScreenBuffer::getNy(float y) const {
    return (int) java::Math::floor((y - getScreenYMin()) / getPixYSize());
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
    dir.inverseScaledCopy((float)distPixel, dir, Numeric::EPSILON_FLOAT);

    double factor = 1.0 / (h * v);

    factor *= distPixel2; // r(eye->pixel)^2
    factor /= java::Math::pow(dir.dotProduct(camera->Z), 2);  // cos^2

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
ScreenBuffer::getBiLinear(float x, float y) const {
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
        radiance[i].scale(factor);
    }

    synced = false;
}

void
ScreenBuffer::setAddScaleFactor(float factor) {
    addFactor = factor;
}

void
ScreenBuffer::setFactor(float factor) {
    factor = factor;
}

void
ScreenBuffer::setRgbImage(bool isRGB) {
    rgbImage = isRGB;
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
