#ifndef __INDXD_CLR_IMG_UNCMPRSSD__
#define __INDXD_CLR_IMG_UNCMPRSSD__

#include <stddef.h>
#include "vsdk/toolkit/common/VSDK.h"
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
    ColorRgb* staticColor;

    int pixelBaseIndex(int x, int y) const;

public:
    static const Class CLASS;
    virtual const Class *getClass() const;

    IndexedColorImageUncompressed(RGBColorPalette* colorTable = NULL);
    virtual ~IndexedColorImageUncompressed();

    virtual int getSizeInBytes() const;

    virtual BOOLEAN init(int width, int height);

    virtual BOOLEAN initNoFill(int width, int height);

    void putPixel(int x, int y, char colorIndex);

    virtual void putPixelRgb(int x, int y, RGBPixel* p);

    char getPixel(int x, int y) const;

    virtual RGBPixel* getPixelRgb(int x, int y) const;

    virtual void getPixelRgb(int x, int y, RGBPixel* p) const;

    virtual int getXSize() const;

    virtual int getYSize() const;

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
