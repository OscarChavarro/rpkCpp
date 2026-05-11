#include <cstring>

#include "java/lang/Math.h"
#include "material/Texture.h"

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

void
Texture::setMonochrome(ColorRgb rgb, float val) {
    rgb.set(val, val, val);
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

    ColorRgb rgb{};

    rgb.clear();
    if ( !data ) {
        return rgb;
    }

    const int pixelIndex00 = (j * width + i) * channels;
    const int pixelIndex01 = (j1 * width + i) * channels;
    const int pixelIndex10 = (j * width + i1) * channels;
    const int pixelIndex11 = (j1 * width + i1) * channels;

    auto channelValue = [this](int pixelIndex, int channel) {
        return static_cast<float>(data[pixelIndex + channel]) / 255.0F;
    };

    ColorRgb rgb00{};
    ColorRgb rgb10{};
    ColorRgb rgb01{};
    ColorRgb rgb11{};

    switch ( channels ) {
        case 1:
            setMonochrome(rgb00, channelValue(pixelIndex00, 0));
            setMonochrome(rgb10, channelValue(pixelIndex10, 0));
            setMonochrome(rgb01, channelValue(pixelIndex01, 0));
            setMonochrome(rgb11, channelValue(pixelIndex11, 0));
            break;
        case 3:
        case 4: {
            rgb00.set(channelValue(pixelIndex00, 0), channelValue(pixelIndex00, 1), channelValue(pixelIndex00, 2));
            rgb10.set(channelValue(pixelIndex10, 0), channelValue(pixelIndex10, 1), channelValue(pixelIndex10, 2));
            rgb01.set(channelValue(pixelIndex01, 0), channelValue(pixelIndex01, 1), channelValue(pixelIndex01, 2));
            rgb11.set(channelValue(pixelIndex11, 0), channelValue(pixelIndex11, 1), channelValue(pixelIndex11, 2));
        }
            break;
        default:
            break;
    }

    rgb.set(
        0.25F * static_cast<float>(u0 * v0 * rgb00.r + u1 * v0 * rgb10.r + u0 * v1 * rgb01.r + u1 * v1 * rgb11.r),
        0.25F * static_cast<float>(u0 * v0 * rgb00.g + u1 * v0 * rgb10.g + u0 * v1 * rgb01.g + u1 * v1 * rgb11.g),
        0.25F * static_cast<float>(u0 * v0 * rgb00.b + u1 * v0 * rgb10.b + u0 * v1 * rgb01.b + u1 * v1 * rgb11.b));
    return rgb;
}
