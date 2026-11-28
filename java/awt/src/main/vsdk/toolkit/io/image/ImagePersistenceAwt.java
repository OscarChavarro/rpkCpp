package vsdk.toolkit.io.image;

// Basic JDK classes
import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.OutputStream;

// Extended JDK classes
import javax.imageio.ImageIO;
import java.awt.image.BufferedImage;

// VSDK classes
import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.media.IndexedColorImageUncompressed;
import vsdk.toolkit.media.Image;
import vsdk.toolkit.media.RGBImageUncompressed;
import vsdk.toolkit.media.RGBAImageUncompressed;
import vsdk.toolkit.media.RGBPixel;
import vsdk.toolkit.render.awt.AwtIndexedColorImageUncompressedRenderer;
import vsdk.toolkit.render.awt.AwtRGBImageUncompressedRenderer;
import vsdk.toolkit.render.awt.AwtRGBAImageUncompressedRenderer;
import java.awt.GraphicsEnvironment;
import java.awt.Transparency;

public class ImagePersistenceAwt extends ImagePersistenceHelper
{
    @Override
    public boolean rgbFormatSupported(String fileExtension)
    {
        return fileExtension.equals("jpg") || fileExtension.equals("jpeg") ||
            fileExtension.equals("gif") || fileExtension.equals("png");
    }

    @Override
    public boolean rgbaFormatSupported(String fileExtension)
    {
        return fileExtension.equals("jpg") || fileExtension.equals("jpeg") ||
            fileExtension.equals("gif") || fileExtension.equals("png");
    }

    @Override
    public boolean indexedColorFormatSupported(String fileExtension)
    {
        return fileExtension.equals("jpg") || fileExtension.equals("jpeg") ||
            fileExtension.equals("gif") || fileExtension.equals("png");
    }

    @Override
    public boolean gifExportSupported()
    {
        return true;
    }

    @Override
    public boolean jpgExportSupported()
    {
        return true;
    }

    @Override
    public boolean pngExportSupported()
    {
        return true;
    }

    @Override
    public IndexedColorImageUncompressed importIndexedColor(File inImageFd) throws ImageNotRecognizedException
    {
        IndexedColorImageUncompressed retImage;
        BufferedImage bi = null;

        try {
            BufferedInputStream bis;
            FileInputStream fis;

            fis = new FileInputStream(inImageFd);
            bis = new BufferedInputStream(fis);
            bi = ImageIO.read(bis);
            bis.close();
            fis.close();
          }
          catch ( Exception e ) {
              Logger.reportMessage(null, VSDK.ERROR, "importRGB (C)",
                                 "Cannot import image file \"" + inImageFd.getAbsolutePath() + "\"");
             throw new ImageNotRecognizedException("Error reading internal file:\n" + e, inImageFd);
        }
        retImage = new IndexedColorImageUncompressed();
        AwtIndexedColorImageUncompressedRenderer.importFromAwtBufferedImage(bi, retImage, inImageFd.getPath());

        return retImage;
    }

    @Override
    public RGBImageUncompressed importRGB(File inImageFd) throws ImageNotRecognizedException, Exception
    {
        RGBImageUncompressed retImage = new RGBImageUncompressed();
        BufferedImage bi = null;

        // OLD SLOW METHOD, DO NOT USE!
        //java.awt.Toolkit awtTools = java.awt.Toolkit.getDefaultToolkit();
        //java.awt.Image image;
        //image = awtTools.getImage(inImageFd.getAbsolutePath());
        //bi = toBufferedImage(image);

        try {
            BufferedInputStream bis;
            FileInputStream fis;

            fis = new FileInputStream(inImageFd);
            bis = new BufferedInputStream(fis);
            bi = ImageIO.read(bis);
            bis.close();
            fis.close();
          }
          catch ( Exception e ) {
              Logger.reportMessage(null, VSDK.ERROR, "importRGB (A)",
                                 "Cannot import image file \"" + inImageFd.getAbsolutePath() + "\"");
             throw new ImageNotRecognizedException("Error reading internal file:\n" + e, inImageFd);
        }
        AwtRGBImageUncompressedRenderer.importFromAwtBufferedImage(bi, retImage /*, inImageFd.getPath()*/);

        return retImage;
    }

    @Override
    public RGBAImageUncompressed importRGBA(File inImageFd) throws ImageNotRecognizedException, Exception
    {
        RGBAImageUncompressed retImage = new RGBAImageUncompressed();
        BufferedImage bi;

        // OLD SLOW METHOD, DO NOT USE!
        //java.awt.Toolkit awtTools = java.awt.Toolkit.getDefaultToolkit();
        //java.awt.Image image;
        //image = awtTools.getImage(inImageFd.getAbsolutePath());
        //bi = toBufferedImage(image);

        try {
            BufferedInputStream bis;
            FileInputStream fis;

            fis = new FileInputStream(inImageFd);
            bis = new BufferedInputStream(fis);
            bi = ImageIO.read(bis);
            AwtRGBAImageUncompressedRenderer.importFromAwtBufferedImage(bi, retImage/*, inImageFd.getPath()*/);
            bis.close();
            fis.close();
            
          }
          catch ( Exception e ) {
            Logger.reportMessageWithException(null, VSDK.ERROR, "ImagePersistenAwt.importRGBA",
                "Cannot import image file \"" + inImageFd.getAbsolutePath() + "\"",
                e);
            return null;
        }

        return retImage;
    }

    //=================================================================
    // DESACTIVATED METHODS!!! DO NOT USE!!!
    //=================================================================
/*
    private boolean hasAlpha(java.awt.Image image) 
    {
        if (image instanceof BufferedImage) 
        {
            BufferedImage bimage = (BufferedImage)image;
            return bimage.getColorModel().hasAlpha();
        }
    
        java.awt.image.PixelGrabber pg;
        pg = new java.awt.image.PixelGrabber(image, 0, 0, 1, 1, false);
        try 
        {
            pg.grabPixels();
        } 
        catch (InterruptedException e) 
        {
        }
    
        java.awt.image.ColorModel cm = pg.getColorModel();
        return cm.hasAlpha();
    }

    private BufferedImage toBufferedImage(java.awt.Image image) 
    {
        if ( image instanceof BufferedImage ) {
            return (BufferedImage)image;
        }
        //System.out.println(image.getClass().getName());
    
        // This code ensures that all the pixels in the image are loaded
        image = new javax.swing.ImageIcon(image).getImage();
    
        // Determine if the image has transparent pixels; for this method's
        // implementation, see e661 Determining If an Image Has Transparent Pixels
        boolean hasAlpha = hasAlpha(image); 
    
        // Create a buffered image with a format that's compatible with the screen
        BufferedImage bimage = null;
        java.awt.GraphicsEnvironment ge;
        ge = GraphicsEnvironment.getLocalGraphicsEnvironment();
        try 
        {
            // Determine the type of transparency of the new buffered image
            int transparency = Transparency.OPAQUE;
            if ( hasAlpha ) {
                transparency = Transparency.BITMASK;
            }
    
            // Create the buffered image
            java.awt.GraphicsDevice gs = ge.getDefaultScreenDevice();
            java.awt.GraphicsConfiguration gc = gs.getDefaultConfiguration();
            bimage = gc.createCompatibleImage(image.getWidth(null), image.getHeight(null), transparency);
        } 
        catch ( java.awt.HeadlessException e ) {
            // The system does not have a screen
        }
    
        if (bimage == null) 
        {
            // Create a buffered image using the default color model
            int type = BufferedImage.TYPE_INT_RGB;
            if (hasAlpha) 
            {
                type = BufferedImage.TYPE_INT_ARGB;
            }
            bimage = new BufferedImage(image.getWidth(null), image.getHeight(null), type);
        }
    
        // Copy image to buffered image
        java.awt.Graphics g = bimage.createGraphics();
    
        // Paint the image onto the buffered image
        g.drawImage(image, 0, 0, null);
        g.dispose();

        return bimage;
    }
*/

    @Override
    public void exportJPG(OutputStream os, Image img)
        throws Exception
    {
        BufferedImage bimg;
        int x, y, xSize, ySize;
        RGBPixel p;

        xSize = img.getXSize();
        ySize = img.getYSize();
        bimg =  new BufferedImage(xSize, ySize, BufferedImage.TYPE_INT_RGB);
        for ( y = 0; y < ySize; y++ ) {
            for ( x = 0; x < xSize; x++ ) {
                p = img.getPixelRgb(x, y);
                bimg.setRGB(x, y, 
                  (VSDK.signedByte2unsignedInteger(p.r)) * 256 * 256 +
                  (VSDK.signedByte2unsignedInteger(p.g)) * 256 +
                  (VSDK.signedByte2unsignedInteger(p.b))
                );
            }
        }

        ImageIO.write(bimg, "jpg", os);

        // OLD DEPRECATED API, DO NOT USE!
        //com.sun.image.codec.jpeg.JPEGImageEncoder jpeg;
        //jpeg = com.sun.image.codec.jpeg.JPEGCodec.createJPEGEncoder(fos);
        //jpeg.encode(bimg);
    }

    /**
    This method writes the contents of the specified image to a file in 
    binary PNG image format. Returns true if everything
    works fine, false if something fails, like a permission access denied
    or if storage device runs out of space.
    @param fd
    @param img
    @return 
    */    
    @Override
    public boolean exportPNG(File fd, Image img)
    {
        try {
            FileOutputStream fos;
            fos = new FileOutputStream(fd);

            exportPNG(fos, img);

            fos.close();
        }
        catch ( Exception e ) {
            return false;
        }
        return true;
    }

    @Override
    public void exportPNG_24bitRgb(OutputStream os, Image img)
        throws Exception
    {
        BufferedImage bimg;
        int x, y, xSize, ySize;
        RGBPixel p;

        xSize = img.getXSize();
        ySize = img.getYSize();
        bimg =  new BufferedImage(xSize, ySize, BufferedImage.TYPE_INT_RGB);
        for ( y = 0; y < ySize; y++ ) {
            for ( x = 0; x < xSize; x++ ) {
                p = img.getPixelRgb(x, y);
                bimg.setRGB(x, y, 
                  (VSDK.signedByte2unsignedInteger(p.r)) * 256 * 256 +
                  (VSDK.signedByte2unsignedInteger(p.g)) * 256 +
                  (VSDK.signedByte2unsignedInteger(p.b))
                );
            }
        }

        ImageIO.write(bimg, "png", os);
    }

    @Override
    public void exportPNG(OutputStream os, Image img)
        throws Exception
    {
/*
        if ( img instanceof IndexedColorImageUncompressed ) {
//NOT WORKING
            exportPNG_8bitGrayscale(os, (IndexedColorImageUncompressed)img);
        }
        else {
*/
        exportPNG_24bitRgb(os, img);
    }

    /**
    This method writes the contents of the specified image to a file in 
    binary GIF image format. Returns true if everything
    works fine, false if something fails, like a permission access denied
    or if storage device runs out of space.
    @param fd
    @param img
    @return 
    */    
    @Override
    public boolean exportGIF(File fd, Image img)
    {
        try {
            BufferedImage bimg;
            int x, y, xSize, ySize;
            RGBPixel p;

            xSize = img.getXSize();
            ySize = img.getYSize();
            bimg =  new BufferedImage(xSize, ySize, BufferedImage.TYPE_INT_RGB);
            for ( y = 0; y < ySize; y++ ) {
                for ( x = 0; x < xSize; x++ ) {
                    p = img.getPixelRgb(x, y);
                    bimg.setRGB(x, y, 
                      (VSDK.signedByte2unsignedInteger(p.r)) * 256 * 256 +
                      (VSDK.signedByte2unsignedInteger(p.g)) * 256 +
                      (VSDK.signedByte2unsignedInteger(p.b))
                    );
                }
            }

            FileOutputStream fos;
            fos = new FileOutputStream(fd);

            ImageIO.write(bimg, "gif", fos);

            // OLD DEPRECATED API, DO NOT USE!
            //com.sun.image.codec.jpeg.JPEGImageEncoder jpeg;
            //jpeg = com.sun.image.codec.jpeg.JPEGCodec.createJPEGEncoder(fos);
            //jpeg.encode(bimg);

            fos.close();
        }
        catch ( Exception e ) {
            return false;
        }
        return true;
    }

}
