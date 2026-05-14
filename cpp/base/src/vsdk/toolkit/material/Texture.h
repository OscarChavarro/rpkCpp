#ifndef TEXTURE__
#define TEXTURE__

#include "vsdk/toolkit/common/color/ColorRgb.h"

class Texture {
  private:
    int width;
    int height;
    int channels;
    unsigned char *data; // First bytes correspond to bottom-left pixel (as in OpenGL)

  public:
    Texture();
    Texture(int inWidth, int inHeight, int inChannels, const unsigned char *inData);
    ~Texture();

    Texture(const Texture &) = delete;
    Texture &operator=(const Texture &) = delete;

    int getWidth() const;
    int getHeight() const;
    int getChannels() const;
    const unsigned char *getData() const;

    ColorRgb evaluateColor(float u, float v) const;
};

inline int
Texture::getWidth() const {
    return width;
}

inline int
Texture::getHeight() const {
    return height;
}

inline int
Texture::getChannels() const {
    return channels;
}

inline const unsigned char *
Texture::getData() const {
    return data;
}

#endif
