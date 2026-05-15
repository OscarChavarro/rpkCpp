#include <string.h>

#include "java/lang/Math.h"
#include "material/Texture.h"

static float
textureChannelValue(const unsigned char *textureData, int pixelIndex, int channel) {
    return ((float)(textureData[pixelIndex + channel])) / 255.0f;
}

Texture::Texture():
    width(0),
    height(0),
    channels(0),
    data(NULL)
{
}

Texture::Texture(
    int inWidth,
    int inHeight,
    int inChannels,
    const unsigned char *inData):
    width(inWidth),
    height(inHeight),
    channels(inChannels),
    data(NULL)
{
    const long byteCount = ((long)(width))
                              * ((long)(height))
                              * ((long)(channels));
    if ( byteCount <= 0 || inData == NULL ) {
        return;
    }
    data = new unsigned char[byteCount];
    memcpy(data, inData, ((size_t)(byteCount)));
}

Texture::~Texture() {
    delete[] data;
    data = NULL;
}

void
Texture::setMonochrome(ColorRgb rgb, float val) {
    rgb.set(val, val, val);
}

ColorRgb
Texture::evaluateColor(float u, float v) const {
    double u1 = u - Math::floor(u);
    double u0 = 1.0 - u1;
    double v1 = v - Math::floor(v);
    double v0 = 1.0 - v1;
    int i = ((int)(u1 * width));
    int i1 = i + 1;
    int j = ((int)(v1 * height));
    int j1 = j + 1;
    if ( i < 0 ) {
        i = 0;
    }
    if ( i >= width ) {
        i = width - 1;
    }
    if ( j < 0 ) {
        j = 0;
    }
    if ( j >= height ) {
        j = height - 1;
    }
    if ( i1 >= width ) {
        i1 -= width;
    }
    if ( j1 >= height ) {
        j1 -= height;
    }

    ColorRgb rgb = ColorRgb();

    rgb.clear();
    if ( !data ) {
        return rgb;
    }

    const int pixelIndex00 = (j * width + i) * channels;
    const int pixelIndex01 = (j1 * width + i) * channels;
    const int pixelIndex10 = (j * width + i1) * channels;
    const int pixelIndex11 = (j1 * width + i1) * channels;

    ColorRgb rgb00 = ColorRgb();
    ColorRgb rgb10 = ColorRgb();
    ColorRgb rgb01 = ColorRgb();
    ColorRgb rgb11 = ColorRgb();

    switch ( channels ) {
        case 1:
            setMonochrome(rgb00, textureChannelValue(data, pixelIndex00, 0));
            setMonochrome(rgb10, textureChannelValue(data, pixelIndex10, 0));
            setMonochrome(rgb01, textureChannelValue(data, pixelIndex01, 0));
            setMonochrome(rgb11, textureChannelValue(data, pixelIndex11, 0));
            break;
        case 3:
        case 4: {
            rgb00.set(textureChannelValue(data, pixelIndex00, 0), textureChannelValue(data, pixelIndex00, 1), textureChannelValue(data, pixelIndex00, 2));
            rgb10.set(textureChannelValue(data, pixelIndex10, 0), textureChannelValue(data, pixelIndex10, 1), textureChannelValue(data, pixelIndex10, 2));
            rgb01.set(textureChannelValue(data, pixelIndex01, 0), textureChannelValue(data, pixelIndex01, 1), textureChannelValue(data, pixelIndex01, 2));
            rgb11.set(textureChannelValue(data, pixelIndex11, 0), textureChannelValue(data, pixelIndex11, 1), textureChannelValue(data, pixelIndex11, 2));
        }
            break;
        default:
            break;
    }

    rgb.set(
        0.25f * ((float)(u0 * v0 * rgb00.r + u1 * v0 * rgb10.r + u0 * v1 * rgb01.r + u1 * v1 * rgb11.r)),
        0.25f * ((float)(u0 * v0 * rgb00.g + u1 * v0 * rgb10.g + u0 * v1 * rgb01.g + u1 * v1 * rgb11.g)),
        0.25f * ((float)(u0 * v0 * rgb00.b + u1 * v0 * rgb10.b + u0 * v1 * rgb01.b + u1 * v1 * rgb11.b)));
    return rgb;
}
