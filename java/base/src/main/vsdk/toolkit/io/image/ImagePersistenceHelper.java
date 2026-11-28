package vsdk.toolkit.io.image;

// Basic java classes
import java.io.File;
import java.io.OutputStream;
import java.io.InputStream;

// VitralSDK classes
import vsdk.toolkit.io.PersistenceElement;
import vsdk.toolkit.media.Image;
import vsdk.toolkit.media.RGBImageUncompressed;
import vsdk.toolkit.media.RGBAImageUncompressed;
import vsdk.toolkit.media.IndexedColorImageUncompressed;

/**
An ImagePersistenceHelper is any class that implements an specific functionality
to aid the main ImagePersistence class, and that it depends on some
library API as such JOGL or Awt/Swing. The design pattern stablishes a bridge
between ImagePersistence and available APIs. Each derived class from
this class implements a wrapper to legacy code.
*/
public abstract class ImagePersistenceHelper extends  PersistenceElement
{
    public boolean rgbFormatFromInputStreamSupported(String fileExtension)
    {
        return false;
    }

    public boolean rgbaFormatFromInputStreamSupported(String fileExtension)
    {
        return false;
    }

    public boolean rgbFormatSupported(String fileExtension)
    {
        return false;
    }

    public boolean rgbaFormatSupported(String fileExtension)
    {
        return false;
    }

    public boolean indexedColorFormatSupported(String fileExtension)
    {
        return false;
    }

    public boolean gifExportSupported()
    {
        return false;
    }

    public boolean jpgExportSupported()
    {
        return false;
    }

    public boolean pngExportSupported()
    {
        return false;
    }

    public RGBImageUncompressed importRGB(File inImageFd) throws ImageNotRecognizedException, Exception
    {
        throw new ImageNotRecognizedException("Not implemented in helper", null);
    }

    public RGBImageUncompressed importRGB(InputStream is) throws ImageNotRecognizedException, Exception
    {
        throw new ImageNotRecognizedException("Not implemented in helper", null);
    }

    public RGBAImageUncompressed importRGBA(InputStream is) throws ImageNotRecognizedException, Exception
    {
        throw new ImageNotRecognizedException("Not implemented in helper", null);
    }

    public RGBAImageUncompressed importRGBA(File inImageFd) throws ImageNotRecognizedException, Exception
    {
        throw new ImageNotRecognizedException("Not implemented in helper", null);
    }

    public IndexedColorImageUncompressed importIndexedColor(File inImageFd) throws ImageNotRecognizedException, Exception
    {
        throw new ImageNotRecognizedException("Not implemented in helper", null);
    }

    public boolean exportGIF(File fd, Image img) {
        return false;
    }

    public void exportJPG(OutputStream os, Image img)
        throws Exception
    {
        throw new Exception("No helper supported");
    }

    public boolean exportPNG(File fd, Image img)
    {
        return false;
    }

    public void exportPNG_24bitRgb(OutputStream os, Image img)
        throws Exception
    {
        throw new Exception("No helper supported");
    }

    public void exportPNG(OutputStream os, Image img)
        throws Exception
    {
        throw new Exception("No helper supported");
    }
}
