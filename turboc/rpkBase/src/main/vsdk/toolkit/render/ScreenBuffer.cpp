#include <string.h>

#include "java/lang/Math.h"
#include "vsdk/toolkit/skin/Numeric.h"
#include "java/lang/System.h"
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
            radiance[i] = ColorRgb(0.0, 0.0, 0.0);
            rgbColor[i] = ColorRgb(0.0, 0.0, 0.0);
        }
    }

    // Clear
    ColorRgb black(0.0, 0.0, 0.0);
    for ( int i = 0; i < camera.xSize * camera.ySize; i++ ) {
        radiance[i] = ColorRgb(0.0, 0.0, 0.0);
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
        Logger::reportMessage("ScreenBuffer", Logger::ERROR, "ScreenBuffer::merge", "Incompatible screen buffer sources");
        return;
    }

    int N = getVRes() * getHRes();

    for ( int i = 0; i < N; i++ ) {
        radiance[i] = ColorRgb(
            src1->radiance[i].getR() + src2->radiance[i].getR(),
            src1->radiance[i].getG() + src2->radiance[i].getG(),
            src1->radiance[i].getB() + src2->radiance[i].getB());
    }
}

void
ScreenBuffer::add(int x, int y, ColorRgb inRadiance) {
    int index = x + (camera.ySize - y - 1) * camera.xSize;

    radiance[index] = ColorRgb(
        radiance[index].getR() + addFactor * inRadiance.getR(),
        radiance[index].getG() + addFactor * inRadiance.getG(),
        radiance[index].getB() + addFactor * inRadiance.getB());
    synced = false;
}

void
ScreenBuffer::set(int x, int y, ColorRgb inRadiance) {
    int index = x + (camera.ySize - y - 1) * camera.xSize;
    radiance[index] = ColorRgb(
        addFactor * inRadiance.getR(),
        addFactor * inRadiance.getG(),
        addFactor * inRadiance.getB());
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

ColorRgb
ScreenBuffer::gammaCorrect(const ColorRgb &rgb, const double gamma[3]) {
    return ColorRgb(
        gamma[0] == 1.0 ? rgb.getR() : Math::pow(rgb.getR(), 1.0 / gamma[0]),
        gamma[1] == 1.0 ? rgb.getG() : Math::pow(rgb.getG(), 1.0 / gamma[1]),
        gamma[2] == 1.0 ? rgb.getB() : Math::pow(rgb.getB(), 1.0 / gamma[2]));
}

/**
Writes the image on the target: the radiances themselves for a high dynamic
range format, or else display RGB values (radiances tone mapped, then gamma
corrected). The rows are written from the top of the image, the last row of
this buffer.
*/
void
ScreenBuffer::writeFile(const ImageOutputTarget *target) {
    if ( target == NULL ) {
        return;
    }

    if ( !synced ) {
        sync();
    }

    System::err.printf("Writing %s file ... ", target->getDriverName());

    const int width = camera.xSize;
    const int height = camera.ySize;
    if ( target->getFormat() == ImageOutputTarget::RADIANCE_RGBE ) {
        if ( isRgbImage() ) {
            System::err.printf("%s does not support display RGB output.\n", target->getDriverName());
        } else {
            RGBImageFloatUncompressed image;
            if ( image.initNoFill(width, height) ) {
                for ( int row = 0; row < height; row++ ) {
                    const int i = height - 1 - row;
                    for ( int x = 0; x < width; x++ ) {
                        image.putPixel(x, row, radiance[i * width + x]);
                    }
                }
                target->writeRadianceImage(&image);
            }
        }
    } else {
        const ToneMappingContext &activeToneMapOptions = requireToneMappingContext();
        // For default radiance -> display RGB
        double gamma[3];
        gamma[0] = activeToneMapOptions.gamma.getR();
        gamma[1] = activeToneMapOptions.gamma.getG();
        gamma[2] = activeToneMapOptions.gamma.getB();
        RGBImageUncompressed image;
        if ( image.initNoFill(width, height) ) {
            for ( int row = 0; row < height; row++ ) {
                const int i = height - 1 - row;
                for ( int x = 0; x < width; x++ ) {
                    ColorRgb displayRgb = radiance[i * width + x];
                    if ( !isRgbImage() ) {
                        // Convert RGB radiance to display RGB
                        ToneMap::radianceToRgb(radiance[i * width + x], &displayRgb, activeToneMapOptions);
                    }
                    displayRgb = gammaCorrect(displayRgb, gamma);
                    image.putPixel(
                        x,
                        row,
                        ((char)((unsigned char)(displayRgb.getR() * 255.0))),
                        ((char)((unsigned char)(displayRgb.getG() * 255.0))),
                        ((char)((unsigned char)(displayRgb.getB() * 255.0))));
                }
            }
            target->writeDisplayImage(&image);
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
    ColorRgb tmpRad = ColorRgb(0.0, 0.0, 0.0);
    const ToneMappingContext &activeToneMapOptions = requireToneMappingContext();

    for ( int i = 0; i < camera.xSize * camera.ySize; i++ ) {
        tmpRad = ColorRgb(
            factor * radiance[i].getR(),
            factor * radiance[i].getG(),
            factor * radiance[i].getB());
        if ( !isRgbImage() ) {
            ToneMap::radianceToRgb(tmpRad, &rgbColor[i], activeToneMapOptions);
        } else {
            tmpRad = ColorRgb(rgbColor[i].getR(), rgbColor[i].getG(), rgbColor[i].getB());
        }
    }

    synced = true;
}


void
ScreenBuffer::syncLine(int lineNumber) {
    ColorRgb tmpRad = ColorRgb(0.0, 0.0, 0.0);
    const ToneMappingContext &activeToneMapOptions = requireToneMappingContext();

    for ( int i = 0; i < camera.xSize; i++ ) {
        tmpRad = ColorRgb(
            factor * radiance[lineNumber * camera.xSize + i].getR(),
            factor * radiance[lineNumber * camera.xSize + i].getG(),
            factor * radiance[lineNumber * camera.xSize + i].getB());
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
    return -camera.pixelWidth * ((double)(camera.xSize)) / 2.0;
}

double
ScreenBuffer::getScreenYMin() const {
    return -camera.pixelHeight * ((double)(camera.ySize)) / 2.0;
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
    return Vector2Dd(
        getScreenXMin() + (((double)(nx)) + xOffset) * getPixXSize(),
        getScreenYMin() + (((double)(ny)) + yOffset) * getPixYSize());
}

Vector2Dd
ScreenBuffer::getPixelCenter(int nx, int ny) const {
    return getPixelPoint(nx, ny, 0.5, 0.5);
}

int
ScreenBuffer::getNx(double x) const {
    return ((int)(Math::floor((x - getScreenXMin()) / getPixXSize())));
}

int
ScreenBuffer::getNy(double y) const {
    return ((int)(Math::floor((y - getScreenYMin()) / getPixYSize())));
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
    Vector2Dd pix = getPixelPoint(nx, ny, xOffset, yOffset);
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
    double h = camera->pixelWidth;
    double v = camera->pixelHeight;

    double x = -h * camera->xSize / 2.0 + pixX * h;
    double y = -v * camera->ySize / 2.0 + pixY * v;

    double xSample = x + h * 0.5;  // (pixX, pixY) indicate upper left
    double ySample = y + v * 0.5;

    dir = camera->Z.add(camera->X.multiply(((double)(xSample)))).add(camera->Y.multiply(((double)(ySample))));
    double distPixel2 = dir.lengthSquared();
    double distPixel = Math::sqrt(distPixel2);
    dir = dir.multiplyByInverse(((double)(distPixel)), Numeric::EPSILON);

    double factor = 1.0 / (h * v);

    factor *= distPixel2; // r(eye->pixel)^2
    factor /= Math::pow(dir.dotProduct(camera->Z), 2);  // cos^2

    return ((double)(factor));
}

double
ScreenBuffer::getScreenXMax() const {
    return camera.pixelWidth * ((double)(camera.xSize)) / 2.0;
}

double
ScreenBuffer::getScreenYMax() const {
    return camera.pixelHeight * ((double)(camera.ySize)) / 2.0;
}

ColorRgb
ScreenBuffer::getBiLinear(double x, double y) const {
    int nx0;
    int nx1;
    int ny0;
    int ny1;
    Vector2Dd center;
    ColorRgb color = ColorRgb(0.0, 0.0, 0.0);

    getPixel(x, y, &nx0, &ny0);
    center = getPixelCenter(nx0, ny0);

    x = (x - center.x) / getPixXSize();
    y = (y - center.y) / getPixYSize();

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

    color = ColorRgb(
        (1.0 - x) * (1.0 - y) * c0.getR() + x * (1.0 - y) * c1.getR() + x * y * c2.getR() + (1.0 - x) * y * c3.getR(),
        (1.0 - x) * (1.0 - y) * c0.getG() + x * (1.0 - y) * c1.getG() + x * y * c2.getG() + (1.0 - x) * y * c3.getG(),
        (1.0 - x) * (1.0 - y) * c0.getB() + x * (1.0 - y) * c1.getB() + x * y * c2.getB() + (1.0 - x) * y * c3.getB());

    return color;
}

void
ScreenBuffer::scaleRadiance(double inFactor) {
    for ( int i = 0; i < camera.xSize * camera.ySize; i++ ) {
        radiance[i] = ColorRgb(
            inFactor * radiance[i].getR(),
            inFactor * radiance[i].getG(),
            inFactor * radiance[i].getB());
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
ScreenBuffer::writeFile(const char *fileName, OutputStream *outputStream, int isPipe) {
    if ( outputStream == NULL ) {
        return;
    }

    const ImageOutputTarget *target = ImageOutputTarget::create(fileName, outputStream, isPipe);
    writeFile(target);
    delete target;
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
