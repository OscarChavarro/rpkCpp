#include <cstring>

#include "java/lang/Math.h"
#include "vsdk/toolkit/material/Texture.h"

Texture::Texture():
    width(0),
    height(0),
    channels(0),
    data(nullptr)
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
    data(nullptr)
{
    const long long byteCount = static_cast<long long>(width)
                              * static_cast<long long>(height)
                              * static_cast<long long>(channels);
    if ( byteCount <= 0 || inData == nullptr ) {
        return;
    }
    data = new unsigned char[byteCount];
    std::memcpy(data, inData, static_cast<size_t>(byteCount));
}

Texture::~Texture() {
    delete[] data;
    data = nullptr;
}

ColorRgb
Texture::evaluateColor(float u, float v) const {
    double u1 = u - java::Math::floor(u);
    double u0 = 1.0 - u1;
    double v1 = v - java::Math::floor(v);
    double v0 = 1.0 - v1;
    int i = static_cast<int>(u1 * width);
    int i1 = i + 1;
    int j = static_cast<int>(v1 * height);
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
        return {0.0, 0.0, 0.0};
    }

    const int pixelIndex00 = (j * width + i) * channels;
    const int pixelIndex01 = (j1 * width + i) * channels;
    const int pixelIndex10 = (j * width + i1) * channels;
    const int pixelIndex11 = (j1 * width + i1) * channels;

    auto channelValue = [this](int pixelIndex, int channel) {
        return static_cast<float>(data[pixelIndex + channel]) / 255.0F;
    };

    double r00 = 0.0;
    double g00 = 0.0;
    double b00 = 0.0;
    double r10 = 0.0;
    double g10 = 0.0;
    double b10 = 0.0;
    double r01 = 0.0;
    double g01 = 0.0;
    double b01 = 0.0;
    double r11 = 0.0;
    double g11 = 0.0;
    double b11 = 0.0;

    switch ( channels ) {
        case 1:
            r00 = g00 = b00 = channelValue(pixelIndex00, 0);
            r10 = g10 = b10 = channelValue(pixelIndex10, 0);
            r01 = g01 = b01 = channelValue(pixelIndex01, 0);
            r11 = g11 = b11 = channelValue(pixelIndex11, 0);
            break;
        case 3:
        case 4: {
            r00 = channelValue(pixelIndex00, 0);
            g00 = channelValue(pixelIndex00, 1);
            b00 = channelValue(pixelIndex00, 2);
            r10 = channelValue(pixelIndex10, 0);
            g10 = channelValue(pixelIndex10, 1);
            b10 = channelValue(pixelIndex10, 2);
            r01 = channelValue(pixelIndex01, 0);
            g01 = channelValue(pixelIndex01, 1);
            b01 = channelValue(pixelIndex01, 2);
            r11 = channelValue(pixelIndex11, 0);
            g11 = channelValue(pixelIndex11, 1);
            b11 = channelValue(pixelIndex11, 2);
        }
            break;
        default:
            break;
    }

    const double r = 0.25 * (u0 * v0 * r00 + u1 * v0 * r10 + u0 * v1 * r01 + u1 * v1 * r11);
    const double g = 0.25 * (u0 * v0 * g00 + u1 * v0 * g10 + u0 * v1 * g01 + u1 * v1 * g11);
    const double b = 0.25 * (u0 * v0 * b00 + u1 * v0 * b10 + u0 * v1 * b01 + u1 * v1 * b11);
    return {r, g, b};
}
