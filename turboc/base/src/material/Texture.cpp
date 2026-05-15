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

    if ( !data ) {
        return ColorRgb(0.0f, 0.0f, 0.0f);
    }

    const int pixelIndex00 = (j * width + i) * channels;
    const int pixelIndex01 = (j1 * width + i) * channels;
    const int pixelIndex10 = (j * width + i1) * channels;
    const int pixelIndex11 = (j1 * width + i1) * channels;

    float r00 = 0.0f;
    float g00 = 0.0f;
    float b00 = 0.0f;
    float r10 = 0.0f;
    float g10 = 0.0f;
    float b10 = 0.0f;
    float r01 = 0.0f;
    float g01 = 0.0f;
    float b01 = 0.0f;
    float r11 = 0.0f;
    float g11 = 0.0f;
    float b11 = 0.0f;

    switch ( channels ) {
        case 1:
            r00 = g00 = b00 = textureChannelValue(data, pixelIndex00, 0);
            r10 = g10 = b10 = textureChannelValue(data, pixelIndex10, 0);
            r01 = g01 = b01 = textureChannelValue(data, pixelIndex01, 0);
            r11 = g11 = b11 = textureChannelValue(data, pixelIndex11, 0);
            break;
        case 3:
        case 4: {
            r00 = textureChannelValue(data, pixelIndex00, 0);
            g00 = textureChannelValue(data, pixelIndex00, 1);
            b00 = textureChannelValue(data, pixelIndex00, 2);
            r10 = textureChannelValue(data, pixelIndex10, 0);
            g10 = textureChannelValue(data, pixelIndex10, 1);
            b10 = textureChannelValue(data, pixelIndex10, 2);
            r01 = textureChannelValue(data, pixelIndex01, 0);
            g01 = textureChannelValue(data, pixelIndex01, 1);
            b01 = textureChannelValue(data, pixelIndex01, 2);
            r11 = textureChannelValue(data, pixelIndex11, 0);
            g11 = textureChannelValue(data, pixelIndex11, 1);
            b11 = textureChannelValue(data, pixelIndex11, 2);
        }
            break;
        default:
            break;
    }

    const float r = 0.25f * ((float)(u0 * v0 * r00 + u1 * v0 * r10 + u0 * v1 * r01 + u1 * v1 * r11));
    const float g = 0.25f * ((float)(u0 * v0 * g00 + u1 * v0 * g10 + u0 * v1 * g01 + u1 * v1 * g11));
    const float b = 0.25f * ((float)(u0 * v0 * b00 + u1 * v0 * b10 + u0 * v1 * b01 + u1 * v1 * b11));
    return ColorRgb(r, g, b);
}
