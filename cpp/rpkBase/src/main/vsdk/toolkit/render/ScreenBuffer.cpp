#include <cstring>

#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/tonemap/ToneMap.h"
#include "vsdk/toolkit/io/wrapper/FileUncompressWrapper.h"
#include "vsdk/toolkit/render/ScreenBuffer.h"
#include "vsdk/toolkit/render/Softids.h"

/**
Constructor : make an screen buffer from a camera definition
*/
ScreenBuffer::ScreenBuffer(const Camera *camera, const Camera *defaultCamera, ToneMappingContext *inToneMapOptions) {
    radiance = nullptr;
    rgbColor = nullptr;
    toneMapOptions = inToneMapOptions;
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
        radiance = new double[3 * camera.xSize * camera.ySize];
        rgbColor = new ColorRgb[camera.xSize * camera.ySize];
        for ( int i = 0; i < camera.xSize * camera.ySize; i++ ) {
            rgbColor[i] = ColorRgb(0.0, 0.0, 0.0);
        }
    }

    // Clear
    const ColorRgb black(0.0, 0.0, 0.0);
    for ( int i = 0; i < camera.xSize * camera.ySize; i++ ) {
        radiance[3 * i] = 0.0;
        radiance[3 * i + 1] = 0.0;
        radiance[3 * i + 2] = 0.0;
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

    memcpy(radiance, source->radiance, 3 * camera.xSize * camera.ySize * sizeof(double));
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
        Logger::reportMessage("ScreenBuffer", Logger::ERROR, "ScreenBuffer::merge", "Incompatible screen buffer sources");
        return;
    }

    const int N = getVRes() * getHRes();

    for ( int i = 0; i < 3 * N; i++ ) {
        radiance[i] = src1->radiance[i] + src2->radiance[i];
    }
}

void
ScreenBuffer::add(int x, int y, const ColorRgb &inRadiance) {
    const int index = 3 * (x + (camera.ySize - y - 1) * camera.xSize);
    radiance[index] = radiance[index] + addFactor * inRadiance.getR();
    radiance[index + 1] = radiance[index + 1] + addFactor * inRadiance.getG();
    radiance[index + 2] = radiance[index + 2] + addFactor * inRadiance.getB();
    synced = false;
}

void
ScreenBuffer::set(int x, int y, const ColorRgb &inRadiance) {
    const int index = 3 * (x + (camera.ySize - y - 1) * camera.xSize);
    radiance[index] = addFactor * inRadiance.getR();
    radiance[index + 1] = addFactor * inRadiance.getG();
    radiance[index + 2] = addFactor * inRadiance.getB();
    synced = false;
}

ColorRgb
ScreenBuffer::get(int x, int y) const {
    const int index = 3 * (x + (camera.ySize - y - 1) * camera.xSize);
    return ColorRgb(radiance[index], radiance[index + 1], radiance[index + 2]);
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

    java::System::err.printf("Writing %s file ... ", ip->driverName);

    const ToneMappingContext &activeToneMapOptions = requireToneMappingContext();
    ip->setToneMappingContext(&activeToneMapOptions);
    ip->gamma[0] = static_cast<float>(activeToneMapOptions.gamma.getR()); // For default radiance -> display RGB
    ip->gamma[1] = static_cast<float>(activeToneMapOptions.gamma.getG());
    ip->gamma[2] = static_cast<float>(activeToneMapOptions.gamma.getB());
    for ( int i = camera.ySize - 1; i >= 0; i-- ) {
        // Write scan lines
        if ( !isRgbImage() ) {
            java::ArrayList<ColorRgb> scanline(camera.xSize > 0 ? camera.xSize : 1);
            for ( int x = 0; x < camera.xSize; x++ ) {
                const int index = 3 * (i * camera.xSize + x);
                scanline.add(ColorRgb(radiance[index], radiance[index + 1], radiance[index + 2]));
            }
            ip->writeRadianceRGB(scanline.data());
        } else {
            java::ArrayList<float> scanline(camera.xSize * 3 > 0 ? camera.xSize * 3 : 1);
            for ( int x = 0; x < camera.xSize; x++ ) {
                const int index = 3 * (i * camera.xSize + x);
                scanline.add(static_cast<float>(radiance[index]));
                scanline.add(static_cast<float>(radiance[index + 1]));
                scanline.add(static_cast<float>(radiance[index + 2]));
            }
            ip->writeDisplayRGB(scanline.data());
        }
    }

    java::System::err.printf("done.\n");
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
    const ToneMappingContext &activeToneMapOptions = requireToneMappingContext();

    for ( int i = 0; i < camera.xSize * camera.ySize; i++ ) {
        if ( !isRgbImage() ) {
            const int index = 3 * i;
            const ColorRgb tmpRad(factor * radiance[index], factor * radiance[index + 1], factor * radiance[index + 2]);
            rgbColor[i] = ToneMap::radianceToRgb(tmpRad, activeToneMapOptions);
        }
    }

    synced = true;
}


void
ScreenBuffer::syncLine(int lineNumber) {
    const ToneMappingContext &activeToneMapOptions = requireToneMappingContext();

    for ( int i = 0; i < camera.xSize; i++ ) {
        if ( !isRgbImage() ) {
            const int index = 3 * (lineNumber * camera.xSize + i);
            const ColorRgb tmpRad(factor * radiance[index], factor * radiance[index + 1], factor * radiance[index + 2]);
            rgbColor[lineNumber * camera.xSize + i] = ToneMap::radianceToRgb(tmpRad, activeToneMapOptions);
        }
    }
}

const ToneMappingContext &
ScreenBuffer::requireToneMappingContext() const {
    if ( toneMapOptions == nullptr ) {
        Logger::reportMessage("ScreenBuffer", Logger::FATAL_ERROR, "ScreenBuffer::requireToneMappingContext", "Tone mapping context not set");
    }
    return *toneMapOptions;
}

void
ScreenBuffer::setToneMappingContext(ToneMappingContext *inToneMapOptions) {
    toneMapOptions = inToneMapOptions;
}

float
ScreenBuffer::getScreenXMin() const {
    return -camera.pixelWidth * static_cast<float>(camera.xSize) / 2.0F;
}

float
ScreenBuffer::getScreenYMin() const {
    return -camera.pixelHeight * static_cast<float>(camera.ySize) / 2.0F;
}

float
ScreenBuffer::getPixXSize() const {
    return camera.pixelWidth;
}

float
ScreenBuffer::getPixYSize() const {
    return camera.pixelHeight;
}

Vector2Df
ScreenBuffer::getPixelPoint(int nx, int ny, float xOffset, float yOffset) const {
    return {getScreenXMin() + (static_cast<float>(nx) + xOffset) * getPixXSize(),
            getScreenYMin() + (static_cast<float>(ny) + yOffset) * getPixYSize()};
}

Vector2Df
ScreenBuffer::getPixelCenter(int nx, int ny) const {
    return getPixelPoint(nx, ny, 0.5, 0.5);
}

int
ScreenBuffer::getNx(float x) const {
    return static_cast<int>(java::Math::floor((x - getScreenXMin()) / getPixXSize()));
}

int
ScreenBuffer::getNy(float y) const {
    return static_cast<int>(java::Math::floor((y - getScreenYMin()) / getPixYSize()));
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
Vector3Df
ScreenBuffer::getPixelVector(int nx, int ny, float xOffset, float yOffset) const {
    const Vector2Df pix = getPixelPoint(nx, ny, xOffset, yOffset);
    Vector3Df dir;
    dir = camera.Z.add(camera.X.multiply(pix.x())).add(camera.Y.multiply(pix.y()));
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
    Vector3Df dir;
    const double h = camera->pixelWidth;
    const double v = camera->pixelHeight;

    const double x = -h * camera->xSize / 2.0 + pixX * h;
    const double y = -v * camera->ySize / 2.0 + pixY * v;

    const double xSample = x + h * 0.5;  // (pixX, pixY) indicate upper left
    const double ySample = y + v * 0.5;

    dir = camera->Z.add(camera->X.multiply(static_cast<float>(xSample))).add(camera->Y.multiply(static_cast<float>(ySample)));
    const double distPixel2 = dir.lengthSquared();
    const double distPixel = java::Math::sqrt(distPixel2);
    dir = dir.multiplyByInverse(static_cast<float>(distPixel), Numeric::EPSILON_FLOAT);

    double factor = 1.0 / (h * v);

    factor *= distPixel2; // r(eye->pixel)^2
    factor /= java::Math::pow(dir.dotProduct(camera->Z), 2);  // cos^2

    return static_cast<float>(factor);
}

float
ScreenBuffer::getScreenXMax() const {
    return camera.pixelWidth * static_cast<float>(camera.xSize) / 2.0F;
}

float
ScreenBuffer::getScreenYMax() const {
    return camera.pixelHeight * static_cast<float>(camera.ySize) / 2.0F;
}

ColorRgb
ScreenBuffer::getBiLinear(float x, float y) const {
    int nx0;
    int nx1;
    int ny0;
    int ny1;
    Vector2Df center;

    getPixel(x, y, &nx0, &ny0);
    center = getPixelCenter(nx0, ny0);

    x = (x - center.x()) / getPixXSize();
    y = (y - center.y()) / getPixYSize();

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

    const ColorRgb c0 = get(nx0, ny0);
    const ColorRgb c1 = get(nx1, ny0);
    const ColorRgb c2 = get(nx1, ny1);
    const ColorRgb c3 = get(nx0, ny1);
    const double c = static_cast<double>(x) * static_cast<double>(y);
    const double bb = static_cast<double>(x) - c;
    const double d = static_cast<double>(y) - c;
    return ColorRgb(c0.getR() + bb * (c1.getR() - c0.getR()) + c * (c2.getR() - c0.getR()) + d * (c3.getR() - c0.getR()),
                    c0.getG() + bb * (c1.getG() - c0.getG()) + c * (c2.getG() - c0.getG()) + d * (c3.getG() - c0.getG()),
                    c0.getB() + bb * (c1.getB() - c0.getB()) + c * (c2.getB() - c0.getB()) + d * (c3.getB() - c0.getB()));
}

void
ScreenBuffer::scaleRadiance(float inFactor) {
    for ( int i = 0; i < 3 * camera.xSize * camera.ySize; i++ ) {
        radiance[i] *= inFactor;
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
ScreenBuffer::writeFile(const char *fileName, java::OutputStream *outputStream, int isPipe) {
    if ( outputStream == nullptr ) {
        return;
    }

    ImageOutputHandle *ip = ImageOutputHandle::createRadianceImageOutputHandle(
        fileName,
        outputStream,
        isPipe,
        camera.xSize,
        camera.ySize);

    writeFile(ip);
    if ( ip != nullptr ) {
        ImageOutputHandle::deleteImageOutputHandle(ip);
    }
}

void
ScreenBuffer::writeFile(const char *fileName) {
    int isPipe = 0;
    java::OutputStream *outputStream = FileUncompressWrapper::openOutputStreamCompressWrapper(fileName, &isPipe);
    if ( outputStream == nullptr ) {
        return;
    }

    writeFile(fileName, outputStream, isPipe);
    FileUncompressWrapper::closeOutputStream(outputStream);
}

#endif
