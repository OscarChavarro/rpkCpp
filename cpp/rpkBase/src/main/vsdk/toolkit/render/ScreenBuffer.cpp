#include <cstring>

#include "java/lang/Math.h"
#include "vsdk/toolkit/skin/Numeric.h"
#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/tonemap/ToneMap.h"
#include "vsdk/toolkit/io/wrapper/FileUncompressWrapper.h"
#include "vsdk/toolkit/media/RGBImageFloatUncompressed.h"
#include "vsdk/toolkit/media/RGBImageUncompressed.h"
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

ColorRgb
ScreenBuffer::gammaCorrect(const ColorRgb &rgb, const double gamma[3]) {
    return ColorRgb(
        gamma[0] == 1.0 ? rgb.getR() : java::Math::pow(rgb.getR(), 1.0 / gamma[0]),
        gamma[1] == 1.0 ? rgb.getG() : java::Math::pow(rgb.getG(), 1.0 / gamma[1]),
        gamma[2] == 1.0 ? rgb.getB() : java::Math::pow(rgb.getB(), 1.0 / gamma[2]));
}

/**
Writes the image on the target: the radiances themselves for a high dynamic
range format, or else display RGB values (radiances tone mapped, then gamma
corrected). The rows are written from the top of the image, the last row of
this buffer.
*/
void
ScreenBuffer::writeFile(const ImageOutputTarget *target) {
    if ( target == nullptr ) {
        return;
    }

    if ( !synced ) {
        sync();
    }

    java::System::err.printf("Writing %s file ... ", target->getDriverName());

    const int width = camera.xSize;
    const int height = camera.ySize;
    if ( target->getFormat() == ImageOutputTarget::Format::RADIANCE_RGBE ) {
        if ( isRgbImage() ) {
            java::System::err.printf("%s does not support display RGB output.\n", target->getDriverName());
        } else {
            RGBImageFloatUncompressed image;
            if ( image.initNoFill(width, height) ) {
                for ( int row = 0; row < height; row++ ) {
                    const int i = height - 1 - row;
                    for ( int x = 0; x < width; x++ ) {
                        const int index = 3 * (i * width + x);
                        image.putPixel(x, row, radiance[index], radiance[index + 1], radiance[index + 2]);
                    }
                }
                target->writeRadianceImage(&image);
            }
        }
    } else {
        const ToneMappingContext &activeToneMapOptions = requireToneMappingContext();
        // For default radiance -> display RGB
        const double gamma[3] = {
            activeToneMapOptions.gamma.getR(),
            activeToneMapOptions.gamma.getG(),
            activeToneMapOptions.gamma.getB()};
        RGBImageUncompressed image;
        if ( image.initNoFill(width, height) ) {
            for ( int row = 0; row < height; row++ ) {
                const int i = height - 1 - row;
                for ( int x = 0; x < width; x++ ) {
                    const int index = 3 * (i * width + x);
                    ColorRgb displayRgb(radiance[index], radiance[index + 1], radiance[index + 2]);
                    if ( !isRgbImage() ) {
                        // Convert RGB radiance to display RGB
                        displayRgb = ToneMap::radianceToRgb(displayRgb, activeToneMapOptions);
                    }
                    displayRgb = gammaCorrect(displayRgb, gamma);
                    image.putPixel(
                        x,
                        row,
                        static_cast<char>(static_cast<unsigned char>(displayRgb.getR() * 255.0)),
                        static_cast<char>(static_cast<unsigned char>(displayRgb.getG() * 255.0)),
                        static_cast<char>(static_cast<unsigned char>(displayRgb.getB() * 255.0)));
                }
            }
            target->writeDisplayImage(&image);
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

double
ScreenBuffer::getScreenXMin() const {
    return -camera.pixelWidth * static_cast<double>(camera.xSize) / 2.0;
}

double
ScreenBuffer::getScreenYMin() const {
    return -camera.pixelHeight * static_cast<double>(camera.ySize) / 2.0;
}

double
ScreenBuffer::getPixXSize() const {
    return camera.pixelWidth;
}

double
ScreenBuffer::getPixYSize() const {
    return camera.pixelHeight;
}

Vector2Dd
ScreenBuffer::getPixelPoint(int nx, int ny, double xOffset, double yOffset) const {
    return {getScreenXMin() + (static_cast<double>(nx) + xOffset) * getPixXSize(),
            getScreenYMin() + (static_cast<double>(ny) + yOffset) * getPixYSize()};
}

Vector2Dd
ScreenBuffer::getPixelCenter(int nx, int ny) const {
    return getPixelPoint(nx, ny, 0.5, 0.5);
}

int
ScreenBuffer::getNx(double x) const {
    return static_cast<int>(java::Math::floor((x - getScreenXMin()) / getPixXSize()));
}

int
ScreenBuffer::getNy(double y) const {
    return static_cast<int>(java::Math::floor((y - getScreenYMin()) / getPixYSize()));
}

void
ScreenBuffer::getPixel(double x, double y, int *nx, int *ny) const {
    *nx = getNx(x);
    *ny = getNy(y);
}

/**
Un-normalized vector pointing from the eye point to the
point with given fractional pixel coordinates
*/
Vector3Dd
ScreenBuffer::getPixelVector(int nx, int ny, double xOffset, double yOffset) const {
    const Vector2Dd pix = getPixelPoint(nx, ny, xOffset, yOffset);
    Vector3Dd dir;
    dir = camera.Z.add(camera.X.multiply(pix.x)).add(camera.Y.multiply(pix.y));
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

double
ScreenBuffer::computeFluxToRadFactor(const Camera *camera, int pixX, int pixY) {
    Vector3Dd dir;
    const double h = camera->pixelWidth;
    const double v = camera->pixelHeight;

    const double x = -h * camera->xSize / 2.0 + pixX * h;
    const double y = -v * camera->ySize / 2.0 + pixY * v;

    const double xSample = x + h * 0.5;  // (pixX, pixY) indicate upper left
    const double ySample = y + v * 0.5;

    dir = camera->Z.add(camera->X.multiply(static_cast<double>(xSample))).add(camera->Y.multiply(static_cast<double>(ySample)));
    const double distPixel2 = dir.lengthSquared();
    const double distPixel = java::Math::sqrt(distPixel2);
    dir = dir.multiplyByInverse(static_cast<double>(distPixel), Numeric::EPSILON);

    double factor = 1.0 / (h * v);

    factor *= distPixel2; // r(eye->pixel)^2
    factor /= java::Math::pow(dir.dotProduct(camera->Z), 2);  // cos^2

    return static_cast<double>(factor);
}

double
ScreenBuffer::getScreenXMax() const {
    return camera.pixelWidth * static_cast<double>(camera.xSize) / 2.0;
}

double
ScreenBuffer::getScreenYMax() const {
    return camera.pixelHeight * static_cast<double>(camera.ySize) / 2.0;
}

ColorRgb
ScreenBuffer::getBiLinear(double x, double y) const {
    int nx0;
    int nx1;
    int ny0;
    int ny1;
    Vector2Dd center;

    getPixel(x, y, &nx0, &ny0);
    center = getPixelCenter(nx0, ny0);

    x = (x - center.x) / getPixXSize();
    y = (y - center.y) / getPixYSize();

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
ScreenBuffer::scaleRadiance(double inFactor) {
    for ( int i = 0; i < 3 * camera.xSize * camera.ySize; i++ ) {
        radiance[i] *= inFactor;
    }

    synced = false;
}

void
ScreenBuffer::setAddScaleFactor(double inFactor) {
    addFactor = inFactor;
}

void
ScreenBuffer::setFactor(double inFactor) {
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

    const ImageOutputTarget *target = ImageOutputTarget::create(fileName, outputStream, isPipe);
    writeFile(target);
    delete target;
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
