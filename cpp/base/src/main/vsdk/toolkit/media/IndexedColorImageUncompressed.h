#ifndef __INDEXED_COLOR_IMAGE_UNCOMPRESSED__
#define __INDEXED_COLOR_IMAGE_UNCOMPRESSED__

#include "vsdk/toolkit/media/Image.h"
class RGBColorPalette;
class RGBAImageUncompressed;
class RGBPixel;
class ColorRgb;

/**
This class represents an indexed color image, where each pixel is stored as
an index into a color palette.
*/
class IndexedColorImageUncompressed : public Image {

private:
    char* data;
    int xSize;
    int ySize;
    RGBColorPalette* colorTable;
    mutable ColorRgb* staticColor;

    int pixelBaseIndex(int x, int y) const;

public:
    IndexedColorImageUncompressed(RGBColorPalette* colorTable = nullptr);
    virtual ~IndexedColorImageUncompressed();

    virtual int getSizeInBytes() const override;

    virtual bool init(int width, int height) override;

    virtual bool initNoFill(int width, int height) override;

    void putPixel(int x, int y, char colorIndex);

    virtual void putPixelRgb(int x, int y, RGBPixel* p) override;

    char getPixel(int x, int y) const;

    virtual RGBPixel* getPixelRgb(int x, int y) const override;

    virtual void getPixelRgb(int x, int y, RGBPixel* p) const override;

    virtual int getXSize() const override;

    virtual int getYSize() const override;

    RGBColorPalette* getColorTable() const;
    void setColorTable(RGBColorPalette* colorTable);

    char* getRawImage() const;

    void setRawImage(int xSize, int ySize, char* data);

    IndexedColorImageUncompressed* clone() const;

    /**
    @return a gray level, translucent (alpha 128) copy of this image, owned
    by the caller
    */
    RGBAImageUncompressed* exportToRgbaImage() const;
};

#endif
