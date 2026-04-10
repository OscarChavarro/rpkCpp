#include <string.h>

#include "java/lang/System.h"
#include "common/Error.h"
#include "common/RenderOptions.h"
#include "tonemap/ToneMap.h"
#include "io/wrapper/FileUncompressWrapper.h"
#include "render/ScreenBuffer.h"
#include "render/Softids.h"

/**
Constructor : make an screen buffer from a camera definition
*/
ScreenBuffer::ScreenBuffer(const Camera *camera, const Camera *defaultCamera, ToneMappingContext *inToneMapOptions) {
    radiance = NULL;
    rgbColor = NULL;
    toneMapOptions = inToneMapOptions;
    init(camera, defaultCamera);
    synced = false;
    factor = 1.0;
    addFactor = 1.0;
    rgbImage = false;
}

ScreenBuffer::~ScreenBuffer() {
    if ( radiance != NULL ) {
        delete[] radiance;
        radiance = NULL;
    }

    if ( rgbColor != NULL ) {
        delete[] rgbColor;
        rgbColor = NULL;
    }
}

bool
ScreenBuffer::isRgbImage() const {
    return rgbImage;
}

void
ScreenBuffer::init(const Camera *inCamera, const Camera *defaultCamera) {
    if ( inCamera == NULL ) {
        // Use the current camera
        inCamera = defaultCamera;
    }

    if ( (radiance != NULL) && (inCamera->xSize != camera.xSize || inCamera->ySize != camera.ySize) ) {
        delete[] rgbColor;
        delete[] radiance;
        radiance = NULL;
    }

    camera = *inCamera;

    if ( radiance == NULL ) {
        radiance = new ColorRgb[camera.xSize * camera.ySize];
        rgbColor = new ColorRgb[camera.xSize * camera.ySize];
        for ( int i = 0; i < camera.xSize * camera.ySize; i++ ) {
            radiance[i].clear();
            rgbColor[i].clear();
        }
    }

    // Clear
    ColorRgb black(0.0, 0.0, 0.0);
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
        Error::error("ScreenBuffer::merge", "Incompatible screen buffer sources");
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

    SoftIds::softRenderPixels(camera.xSize, camera.ySize, rgbColor, requireToneMappingContext());
}

void
ScreenBuffer::writeFile(ImageOutputHandle *ip) {
    if ( !ip ) {
        return;
    }

    if ( !synced ) {
        sync();
    }

    System::err.printf("Writing %s file ... ", ip->driverName);

    const ToneMappingContext &activeToneMapOptions = requireToneMappingContext();
    ip->setToneMappingContext(&activeToneMapOptions);
    ip->gamma[0] = activeToneMapOptions.gamma.r; // For default radiance -> display RGB
    ip->gamma[1] = activeToneMapOptions.gamma.g;
    ip->gamma[2] = activeToneMapOptions.gamma.b;
    for ( int i = camera.ySize - 1; i >= 0; i-- ) {
        // Write scan lines
        if ( !isRgbImage() ) {
            ip->writeRadianceRGB(&radiance[i * camera.xSize]);
        } else {
            ip->writeDisplayRGB(((float *)(&radiance[i * camera.xSize])));
        }
    }

    System::err.printf("done.\n");
}

void
ScreenBuffer::renderScanline(int y) {
    y = camera.ySize - y - 1;

    if ( !synced ) {
        syncLine(y);
    }

    SoftIds::softRenderPixels(camera.xSize, 1, &rgbColor[y * camera.xSize], requireToneMappingContext());
}

void
ScreenBuffer::sync() {
    ColorRgb tmpRad = ColorRgb();
    const ToneMappingContext &activeToneMapOptions = requireToneMappingContext();

    for ( int i = 0; i < camera.xSize * camera.ySize; i++ ) {
        tmpRad.scaledCopy(factor, radiance[i]);
        if ( !isRgbImage() ) {
            ToneMap::radianceToRgb(tmpRad, &rgbColor[i], activeToneMapOptions);
        } else {
            tmpRad.set(rgbColor[i].r, rgbColor[i].g, rgbColor[i].b);
        }
    }

    synced = true;
}


void
ScreenBuffer::syncLine(int lineNumber) {
    ColorRgb tmpRad = ColorRgb();
    const ToneMappingContext &activeToneMapOptions = requireToneMappingContext();

    for ( int i = 0; i < camera.xSize; i++ ) {
        tmpRad.scaledCopy(factor, radiance[lineNumber * camera.xSize + i]);
        if ( !isRgbImage() ) {
            ToneMap::radianceToRgb(tmpRad, &rgbColor[lineNumber * camera.xSize + i], activeToneMapOptions);
        } else {
            tmpRad = rgbColor[lineNumber * camera.xSize + i];
        }
    }
}

const ToneMappingContext &
ScreenBuffer::requireToneMappingContext() const {
    if ( toneMapOptions == NULL ) {
        Error::fatal(-1, "ScreenBuffer::requireToneMappingContext", "Tone mapping context not set");
    }
    return *toneMapOptions;
}

void
ScreenBuffer::setToneMappingContext(ToneMappingContext *inToneMapOptions) {
    toneMapOptions = inToneMapOptions;
}

float
ScreenBuffer::getScreenXMin() const {
    return -camera.pixelWidth * ((float)(camera.xSize)) / 2.0f;
}

float
ScreenBuffer::getScreenYMin() const {
    return -camera.pixelHeight * ((float)(camera.ySize)) / 2.0f;
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
    return Vector2D(
        getScreenXMin() + (((float)(nx)) + xOffset) * getPixXSize(),
        getScreenYMin() + (((float)(ny)) + yOffset) * getPixYSize());
}

Vector2D
ScreenBuffer::getPixelCenter(int nx, int ny) const {
    return getPixelPoint(nx, ny, 0.5, 0.5);
}

int
ScreenBuffer::getNx(float x) const {
    return ((int)(Math::floor((x - getScreenXMin()) / getPixXSize())));
}

int
ScreenBuffer::getNy(float y) const {
    return ((int)(Math::floor((y - getScreenYMin()) / getPixYSize())));
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

#ifdef RAYTRACING_ENABLED

float
ScreenBuffer::computeFluxToRadFactor(const Camera *camera, int pixX, int pixY) {
    Vector3D dir;
    double h = camera->pixelWidth;
    double v = camera->pixelHeight;

    double x = -h * camera->xSize / 2.0 + pixX * h;
    double y = -v * camera->ySize / 2.0 + pixY * v;

    double xSample = x + h * 0.5;  // (pixX, pixY) indicate upper left
    double ySample = y + v * 0.5;

    dir.combine3(camera->Z, ((float)(xSample)), camera->X, ((float)(ySample)), camera->Y);
    double distPixel2 = dir.norm2();
    double distPixel = Math::sqrt(distPixel2);
    dir.inverseScaledCopy(((float)(distPixel)), dir, Numeric::EPSILON_FLOAT);

    double factor = 1.0 / (h * v);

    factor *= distPixel2; // r(eye->pixel)^2
    factor /= Math::pow(dir.dotProduct(camera->Z), 2);  // cos^2

    return ((float)(factor));
}

float
ScreenBuffer::getScreenXMax() const {
    return camera.pixelWidth * ((float)(camera.xSize)) / 2.0f;
}

float
ScreenBuffer::getScreenYMax() const {
    return camera.pixelHeight * ((float)(camera.ySize)) / 2.0f;
}

ColorRgb
ScreenBuffer::getBiLinear(float x, float y) const {
    int nx0;
    int nx1;
    int ny0;
    int ny1;
    Vector2D center;
    ColorRgb color = ColorRgb();

    getPixel(x, y, &nx0, &ny0);
    center = getPixelCenter(nx0, ny0);

    x = (x - center.u) / getPixXSize();
    y = (y - center.v) / getPixYSize();

    if ( x < 0 ) {
        // Point on left side of pixel center
        x = -x;
        nx1 = Math::max(nx0 - 1, 0);
    } else {
        nx1 = Math::min(getHRes(), nx0 + 1);
    }

    if ( y < 0 ) {
        y = -y;
        ny1 = Math::max(ny0 - 1, 0);
    } else {
        ny1 = Math::min(getVRes(), ny0 + 1);
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
ScreenBuffer::scaleRadiance(float inFactor) {
    for ( int i = 0; i < camera.xSize * camera.ySize; i++ ) {
        radiance[i].scale(inFactor);
    }

    synced = false;
}

void
ScreenBuffer::setAddScaleFactor(float inFactor) {
    addFactor = inFactor;
}

void
ScreenBuffer::setFactor(float inFactor) {
    factor = inFactor;
}

void
ScreenBuffer::setRgbImage(bool isRGB) {
    rgbImage = isRGB;
}

void
ScreenBuffer::writeFile(const char *fileName, OutputStream *outputStream, int isPipe) {
    if ( outputStream == NULL ) {
        return;
    }

    ImageOutputHandle *ip = ImageOutputHandle::createRadianceImageOutputHandle(
        fileName,
        outputStream,
        isPipe,
        camera.xSize,
        camera.ySize);

    writeFile(ip);
    if ( ip != NULL ) {
        ImageOutputHandle::deleteImageOutputHandle(ip);
    }
}

void
ScreenBuffer::writeFile(const char *fileName) {
    int isPipe = 0;
    OutputStream *outputStream = FileUncompressWrapper::openOutputStreamCompressWrapper(fileName, &isPipe);
    if ( outputStream == NULL ) {
        return;
    }

    writeFile(fileName, outputStream, isPipe);
    FileUncompressWrapper::closeOutputStream(outputStream);
}

#endif
