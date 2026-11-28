package vsdk.toolkit.io.image;

// Basic JDK classes
import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.StringTokenizer;
import java.util.ArrayList;

// VitralSDK classes
import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.media.Image;
import vsdk.toolkit.media.RGBImageUncompressed;
import vsdk.toolkit.media.RGBImageFloatUncompressed;
import vsdk.toolkit.media.RGBAImageCompressed;
import vsdk.toolkit.media.RGBAImageUncompressed;
import vsdk.toolkit.media.RGBPixel;
import vsdk.toolkit.media.IndexedColorImageUncompressed;
import vsdk.toolkit.io.PersistenceElement;

/**
This class is a front end front which images of various formats can be
exported and/or imported to/from files.

This class follows a Singleton design pattern.

\todo  Does this implements a "Builder" design pattern??? A Factory design
      pattern... possibly some of that combined with a Facade design
      pattern?
 */
public class ImagePersistence extends PersistenceElement
{
    public static boolean nativeWarningGiven = false;
    private static ArrayList<ImagePersistenceHelper> helpers = null;

    static {
        initHelpers();
    }

    private static void loadPluginHelper(String className)
    {
        ClassLoader cl = ImagePersistence.class.getClassLoader();
        try {
            Class<?> handle;
            handle = cl.loadClass(className);
            Object o = handle.getDeclaredConstructor().newInstance();
            if ( o instanceof ImagePersistenceHelper ) {
                helpers.add((ImagePersistenceHelper)o);
            }
        }
        catch ( ClassNotFoundException e ) {
            // Class is not available... (not a problem)
        }
        catch ( ReflectiveOperationException e ) {
        }
    }

    public static void initHelpers()
    {
        if ( helpers == null  ) {
            helpers = new ArrayList<ImagePersistenceHelper>();
        }

        // Call order reflects prefered order
        loadPluginHelper("vsdk.toolkit.io.image.ImagePersistenceAwt");
        loadPluginHelper("vsdk.toolkit.io.image.ImagePersistenceTarga");
        loadPluginHelper("vsdk.toolkit.io.image.ImagePersistenceJogl");
        loadPluginHelper("vsdk.toolkit.io.image.ImagePersistenceAndroid");
    }

    public static void registerHelper(ImagePersistenceHelper h)
    {
        initHelpers();
        helpers.add(h);
    }

    private static RGBAImageUncompressed importDDSRGBA(File inImageFd)
    {
        try {
            int i;
            for ( i = 0; i < helpers.size(); i++ ) {
                if ( helpers.get(i).rgbaFormatSupported("dds") ) {
                    return helpers.get(i).importRGBA(inImageFd);
                }
            }
        }
        catch ( Exception e ) {

        }
        return createNotAvailableImageRGBA();
    }

    private static RGBAImageUncompressed createNotAvailableImageRGBA()
    {
        RGBAImageUncompressed data;
        data = new RGBAImageUncompressed();
        data.init(256, 256);
        data.createTestPattern();
        Logger.reportMessage(null, VSDK.WARNING,
            "ImagePersistence",
            "Helper class not available, returning test RGBA image"
        );
        return data;
    }

    private static RGBImageUncompressed createNotAvailableImageRGB()
    {
        RGBImageUncompressed data;
        data = new RGBImageUncompressed();
        data.init(256, 256);
        data.createTestPattern();
        Logger.reportMessage(null, VSDK.WARNING,
            "ImagePersistence",
            "Helper class not available, returning test RGB image"
        );
        return data;
    }

    private static IndexedColorImageUncompressed createNotAvailableImageIndexedColor()
    {
        IndexedColorImageUncompressed data;
        data = new IndexedColorImageUncompressed();
        data.init(256, 256);
        data.createTestPattern();
        Logger.reportMessage(null, VSDK.WARNING,
            "ImagePersistence",
            "Helper class not available, returning test IndexedColor image"
        );
        return data;
    }

    private static RGBImageUncompressed importDDSRGB(File inImageFd)
    {
        try {
            int i;
            for ( i = 0; i < helpers.size(); i++ ) {
                if ( helpers.get(i).rgbFormatSupported("dds") ) {
                    return helpers.get(i).importRGB(inImageFd);
                }
            }
        }
        catch ( Exception e ) {

        }
        return createNotAvailableImageRGB();
    }

    private static RGBAImageCompressed importDDSCompressed(File inImageFd)
        throws ImageNotRecognizedException, Exception
    {
        byte fileData[];
        fileData = Files.readAllBytes(inImageFd.toPath());

        if ( fileData.length < 128 ) {
            throw new ImageNotRecognizedException("DDS file too short", inImageFd);
        }
        if ( fileData[0] != 'D' || fileData[1] != 'D' ||
             fileData[2] != 'S' || fileData[3] != ' ' ) {
            throw new ImageNotRecognizedException("DDS signature not recognized", inImageFd);
        }

        int headerSize = readIntLE(fileData, 4);
        if ( headerSize != 124 ) {
            throw new ImageNotRecognizedException("DDS header size not recognized", inImageFd);
        }

        int height = readIntLE(fileData, 12);
        int width = readIntLE(fileData, 16);
        int pixelFormatFlags = readIntLE(fileData, 80);
        String fourCC;
        fourCC = new String(fileData, 84, 4, StandardCharsets.US_ASCII);

        if ( (pixelFormatFlags & 0x04) == 0 ) {
            throw new ImageNotRecognizedException("DDS file does not use a FourCC compressed format", inImageFd);
        }

        int compressionFormat = ddsFourCCToCompressionFormat(fourCC);
        if ( compressionFormat == RGBAImageCompressed.COMPRESSION_UNKNOWN ) {
            throw new ImageNotRecognizedException("DDS compressed format not supported: " + fourCC, inImageFd);
        }

        int dataOffset = 128;
        int dataSize = fileData.length - dataOffset;
        byte compressedData[];
        compressedData = new byte[dataSize];
        System.arraycopy(fileData, dataOffset, compressedData, 0, dataSize);

        RGBAImageCompressed image;
        image = new RGBAImageCompressed();
        if ( !image.initCompressed(width, height, compressionFormat, compressedData) ) {
            throw new ImageNotRecognizedException("Could not initialize compressed DDS image", inImageFd);
        }
        return image;
    }

    private static int ddsFourCCToCompressionFormat(String fourCC)
    {
        if ( fourCC.equals("DXT1") ) {
            return RGBAImageCompressed.COMPRESSION_DXT1;
        }
        if ( fourCC.equals("DXT3") ) {
            return RGBAImageCompressed.COMPRESSION_DXT3;
        }
        if ( fourCC.equals("DXT5") ) {
            return RGBAImageCompressed.COMPRESSION_DXT5;
        }
        return RGBAImageCompressed.COMPRESSION_UNKNOWN;
    }

    private static int readIntLE(byte data[], int offset)
    {
        return (data[offset] & 0xFF) |
            ((data[offset + 1] & 0xFF) << 8) |
            ((data[offset + 2] & 0xFF) << 16) |
            ((data[offset + 3] & 0xFF) << 24);
    }

    /**
    Given the filename of an input data file which contains an image, this
    method tries to recognize the file format and load the contents of it
    to the image.

    \todo  Do not assume the file format only from the filename extension,
    but trying to detect file headers.

    @param inImageFd - The file containing the image
    @return An RGBAImageUncompressed entity that contains the image loaded in memory.

    Will change:
      - Choose a better name for this method
      - Do not receive a File, but a Stream of bytes
     * @throws vsdk.toolkit.io.image.ImageNotRecognizedException
    */
    public static RGBAImageUncompressed importRGBA(File inImageFd)
        throws ImageNotRecognizedException, Exception
    {
        String type = extractExtensionFromFile(inImageFd);
        RGBAImageUncompressed retImage;
        retImage = new RGBAImageUncompressed();

        //- Try optimized reading, if native library is available ---------
        if ( NativeImageReaderWrapper.available && type.equals("png") ) {
            _NativeImageReaderWrapperHeaderInfo header;
            header = new _NativeImageReaderWrapperHeaderInfo();
            try {
                NativeImageReaderWrapper.readPngHeader(header,
                    inImageFd.getAbsolutePath());
                retImage.initNoFill((int)header.xSize, (int)header.ySize);
                NativeImageReaderWrapper.readPngDataRGBA(header, retImage.getRawImageDirectBuffer());
                return retImage;
            }
            finally {
                NativeImageReaderWrapper.releasePngHeader(header);
            }
        }

        reportMissingNativeReader(type);

        //-----------------------------------------------------------------
        int i;
        for ( i = 0; i < helpers.size(); i++ ) {
            if ( helpers.get(i).rgbaFormatSupported(type) ) {
                return helpers.get(i).importRGBA(inImageFd);
            }
        }

        if( type.equals("dds") ) {
            //delete retImage;
            return importDDSRGBA(inImageFd);
        }

        throw new ImageNotRecognizedException("Image not recognized", inImageFd);
    }

    /**
    Reports, only once and as a single informative line, that png images are
    being read with the slow AWT based path because the NativeImageReader
    native library is not available. Build it with the CMake project in
    pkgs/NativeImageReader to enable the fast path.
    */
    private static void reportMissingNativeReader(String type)
    {
        if ( nativeWarningGiven || NativeImageReaderWrapper.available ||
             !type.equals("png") ) {
            return;
        }

        nativeWarningGiven = true;

        String diagnostic = NativeImageReaderWrapper.getLoadDiagnostic();

        System.err.println(
            "ImagePersistence: NativeImageReader library not available" +
            (diagnostic != null ? " (" + diagnostic + ")" : "") +
            ", using the AWT based png reader, which can be slow. Build " +
            "pkgs/NativeImageReader to enable the native reader.");
    }

    /**
    Given a text string, this method determines if is a Unix style single
    line comment, that is, if line begins with a '#' character.
    */
    private static boolean
    isTextComment(String line)
    {
        char arr[] = line.toCharArray();
        int i;

        for ( i = 0;
              i < arr.length && (arr[i] != ' ' && arr[i] != '\t');
              i++ ) {
            // Skip comment
        }

        return i < arr.length && arr[i] == '#';
    }

    public static RGBImageUncompressed importRGB(InputStream is) throws ImageNotRecognizedException, Exception
    {
        try {
            int i;
            for ( i = 0; i < helpers.size(); i++ ) {
                if ( helpers.get(i).rgbFormatFromInputStreamSupported("*") ) {
                    return helpers.get(i).importRGB(is);
                }
            }
        }
        catch ( Exception e ) {

        }
        return createNotAvailableImageRGB();
    }

    public static RGBAImageUncompressed importRGBA(InputStream is) throws ImageNotRecognizedException, Exception
    {
        try {
            int i;
            for ( i = 0; i < helpers.size(); i++ ) {
                if ( helpers.get(i).rgbaFormatFromInputStreamSupported("*") ) {
                    return helpers.get(i).importRGBA(is);
                }
            }
        }
        catch ( Exception e ) {
            Logger.reportMessageWithException(null, VSDK.FATAL_ERROR, "ImagePersistence.importRGBA", "INPUT ERROR: ", e);
        }
        return createNotAvailableImageRGBA();
    }

    /**
    Given the filename of an input data file which contains an image, this
    method tries to recognize the file format and load the contents of it
    to the image.

    \todo  Do not assume the file format only from the filename extension,
    but trying to detect file headers.

    @param inImageFd - The file respesenting the image
    @return An RGBImageUncompressed entity that contains the image loaded in memory.

    Will change:
      - Choose a better name for this method
      - Do not recieve a File, but a Stream of bytes
    @throws vsdk.toolkit.io.image.ImageNotRecognizedException
    */
    @SuppressWarnings("unchecked")
    public static <T extends Image> T importRGB(File inImageFd)
        throws ImageNotRecognizedException, Exception
    {
        String type = extractExtensionFromFile(inImageFd);
        RGBImageUncompressed retImage = new RGBImageUncompressed();

        if( type.equals("dds") ) {
            return (T)importDDSCompressed(inImageFd);
        }

        //- Try optimized reading, if native library is available ---------
        if ( NativeImageReaderWrapper.available && type.equals("png") ) {
            _NativeImageReaderWrapperHeaderInfo header;
            header = new _NativeImageReaderWrapperHeaderInfo();
            try {
                NativeImageReaderWrapper.readPngHeader(header,
                    inImageFd.getAbsolutePath());
                retImage.initNoFill((int)header.xSize, (int)header.ySize);
                NativeImageReaderWrapper.readPngDataRGB(header, retImage.getRawImageDirectBuffer());
                return (T)retImage;
            }
            finally {
                NativeImageReaderWrapper.releasePngHeader(header);
            }
        }

        reportMissingNativeReader(type);

        //-----------------------------------------------------------------
        int i;
        for ( i = 0; i < helpers.size(); i++ ) {
            if ( helpers.get(i).rgbFormatSupported(type) ) {
                return (T)helpers.get(i).importRGB(inImageFd);
            }
        }

        if( type.equals("ppm") )  {
            try {
                BufferedInputStream bis;
                FileInputStream fis;

                fis = new FileInputStream(inImageFd);
                bis = new BufferedInputStream(fis);

                boolean exit = false;
                String line;
                int stage = 1;
                int xSize = 0, ySize = 0;

                do {
                    line = readAsciiLine(bis);
                    if ( line.equals("255") ) {
                        exit = true;
                    }
                    if ( isTextComment(line) ) {
                        continue;
                    }
                    switch ( stage ) {
                      case 1: // PPM signature - data type
                        if ( !line.startsWith("P6") ) {
                            throw new ImageNotRecognizedException("Error reading internal PPM file subformat:\n" + line, inImageFd);
                        }
                        stage++;
                        break;
                      case 2:
                        if ( line.startsWith("#") ) {
                            // Skip comment line
                          }
                          else {
                            StringTokenizer parser;
                            parser = new StringTokenizer(line);
                            xSize = Integer.parseInt(parser.nextToken());
                            ySize = Integer.parseInt(parser.nextToken());
                            stage++;
                        }
                        break;
                    }
                } while ( !exit );

                retImage = new RGBImageUncompressed();
                retImage.initNoFill(xSize, ySize);
                //byte barr[] = retImage.getRawImage();
                //readBytes(bis, barr);

                ByteBuffer bb = retImage.getRawImageDirectBuffer();
                byte barr[] = new byte[xSize*3];
                for ( i = 0; i < ySize; i++ ) {
                    readBytes(bis, barr);
                    bb.put(barr);
                }

                //-------------------------------------------------------------
                // Invert image
                int x, y;
                RGBPixel pa = new RGBPixel();
                RGBPixel pb = new RGBPixel();

                for ( y = 0; y < ySize/2; y++ ) {
                    for ( x = 0; x < xSize; x++ ) {
                        retImage.getPixelRgb(x, y, pa);
                        retImage.getPixelRgb(x, ySize-y-1, pb);
                        retImage.putPixelRgb(x, y, pb);
                        retImage.putPixelRgb(x, ySize-y-1, pa);
                    }
                }
                //-------------------------------------------------------------

                bis.close();
                fis.close();
                return (T)retImage;
              }
              catch ( Exception e ) {
                  Logger.reportMessage(null, VSDK.ERROR, "importRGB (B)",
                                     "Cannot import image file \"" + inImageFd.getAbsolutePath() + "\"" + e);
                 throw new ImageNotRecognizedException("Error reading internal file:\n" + e, inImageFd);
            }
        }
        return (T)createNotAvailableImageRGB();
    }

    /**
    Given the filename of an input data file which contains an image, this
    method tries to recognize the file format and load the contents of it
    to the image.

    \todo  Do not assume the file format only from the filename extension,
    but trying to detect file headers.

    @param inImageFd - The file respesenting the image
    @return An IndexedColorImageUncompressed entity that contains the image loaded in memory.

    Will change:
      - Choose a better name for this method
      - Do not recieve a File, but a Stream of bytes
    @throws vsdk.toolkit.io.image.ImageNotRecognizedException
    */
    public static IndexedColorImageUncompressed importIndexedColor(File inImageFd)
        throws ImageNotRecognizedException, Exception
    {
        String type = extractExtensionFromFile(inImageFd);
        IndexedColorImageUncompressed retImage;
        Image img;

        int i;
        for ( i = 0; i < helpers.size(); i++ ) {
            if ( helpers.get(i).indexedColorFormatSupported(type) ) {
                return helpers.get(i).importIndexedColor(inImageFd);
            }
        }

        if( type.equals("bw") ) {
            img = ImagePersistenceSGI.readImageSGI(inImageFd.getAbsolutePath());
            if ( img instanceof IndexedColorImageUncompressed ) {
                retImage = (IndexedColorImageUncompressed)img;
            }
            else {
                throw new ImageNotRecognizedException("Convertion needed",
                inImageFd);
            }
            return retImage;
        }
        throw new ImageNotRecognizedException("Image not recognized", inImageFd);
    }

    private static void transferPixels(int[] ori, byte[] dest, int w, int h, int pixelDepth)
    {
        int bPos=0;
        for(int i=0; i<w*h; i++)
        {
            dest[bPos]=(byte)(ori[i]>>16);
            bPos++;
            dest[bPos]=(byte)(ori[i]>>8);
            bPos++;
            dest[bPos]=(byte)(ori[i]);
            bPos++;
            if(pixelDepth==32)
            {
                dest[bPos]=(byte)(ori[i]>>24);
                bPos++;
            }
        }
    }

    /**
    This method writes the contents of the specified image to a file in
    binary JPEG image format. Returns true if everything
    works fine, false if something fails, like a permission access denied
    or if storage device runs out of space.
    @param fd
    @param img
    @return
    */
    public static boolean exportJPG(File fd, Image img)
    {
        try {
            FileOutputStream fos;
            fos = new FileOutputStream(fd);

            exportJPG(fos, img);

            fos.close();
        }
        catch ( Exception e ) {
            return false;
        }
        return true;
    }

    public static boolean exportGIF(File fd, Image img)
    {
        int i;

        for ( i = 0; i < helpers.size(); i++ ) {
            if ( helpers.get(i).gifExportSupported() ) {
                return helpers.get(i).exportGIF(fd, img);
            }
        }

        Logger.reportMessage(null, VSDK.WARNING,
            "ImagePersistence",
            "Helper class not available, not saving GIF image"
        );
        return false;
    }

    public static void exportJPG(OutputStream os, Image img) throws Exception
    {
        int i;

        for ( i = 0; i < helpers.size(); i++ ) {
            if ( helpers.get(i).jpgExportSupported() ) {
                helpers.get(i).exportJPG(os, img);
                return;
            }
        }
        Logger.reportMessage(null, VSDK.WARNING,
            "ImagePersistence",
            "Helper class not available, not saving JPG image");
    }

    public static void exportPNG(OutputStream os, Image img)
        throws Exception
    {
        int i;
        for ( i = 0; i < helpers.size(); i++ ) {
            if ( helpers.get(i).pngExportSupported() ) {
                helpers.get(i).exportPNG_24bitRgb(os, img);
                return;
            }
        }
        Logger.reportMessage(null, VSDK.WARNING,
            "ImagePersistence",
            "Helper class not available, not saving PNG image");
    }

    public static void exportPNG(File fd, Image img)
    {
        try {
            int i;
            for ( i = 0; i < helpers.size(); i++ ) {
                if ( helpers.get(i).pngExportSupported() ) {
                    helpers.get(i).exportPNG(fd, img);
                    return;
                }
            }
        }
        catch ( Exception e ) {
            Logger.reportMessage(null, VSDK.WARNING,
                "ImagePersistence",
                "Error saving PNG image");
        }
    }

    public static void exportPNG_24bitRgb(OutputStream os, Image img) throws Exception
    {
        int i;
        for ( i = 0; i < helpers.size(); i++ ) {
            if ( helpers.get(i).pngExportSupported() ) {
                helpers.get(i).exportPNG_24bitRgb(os, img);
                return;
            }
        }
        Logger.reportMessage(null, VSDK.WARNING,
            "ImagePersistence",
            "Helper class not available, not saving PNG image");
    }

    /**
    This method writes the contents of the specified image to a file in
    binary RGB PPM format (i.e. P6 PPM sub-format). Returns true if everything
    works fine, false if something fails, like a permission access denied
    or if storage device runs out of space.
    @param fd
    @param img
    @return
    */
    public static boolean exportPPM(File fd, Image img)
    {
        try {
            FileOutputStream fos;
            fos = new FileOutputStream(fd);
            BufferedOutputStream writer;
            writer = new BufferedOutputStream(fos);
            boolean status = exportPPM(writer, img);
            writer.flush();
            writer.close();
            fos.close();
            return status;
        }
        catch (Exception e) {
            return false;
        }
    }

    /**
    Same as exportPPM over a file, but writing on the given stream (e.g. a
    compressed or piped stream), which is not closed.
    @param os
    @param img
    @return true if export was successful, false otherwise
    */
    public static boolean exportPPM(OutputStream os, Image img)
    {
        if ( img == null ) {
            return false;
        }
        try {
            String line1 = "P6\n";
            String line2 = "# Image generated by VitralSDK (https://github.com/OscarChavarro/vitral)\n";
            String line3 = img.getXSize() + " " + img.getYSize() + "\n";
            String line4 = "255\n";
            byte arr[];

            arr = line1.getBytes();
            os.write(arr, 0, arr.length);
            arr = line2.getBytes();
            os.write(arr, 0, arr.length);
            arr = line3.getBytes();
            os.write(arr, 0, arr.length);
            arr = line4.getBytes();
            os.write(arr, 0, arr.length);

            // One row at a time, without allocating a pixel per sample
            int xSize = img.getXSize();
            byte[] row = new byte[3 * xSize];
            RGBPixel p = new RGBPixel();
            int x;
            int y;
            for ( y = 0; y < img.getYSize(); y++ ) {
                for ( x = 0; x < xSize; x++ ) {
                    img.getPixelRgb(x, y, p);
                    row[3 * x] = p.r;
                    row[3 * x + 1] = p.g;
                    row[3 * x + 2] = p.b;
                }
                os.write(row, 0, row.length);
            }
        }
        catch (Exception e) {
            return false;
        }
        return true;
    }

    /**
    This method writes the contents of the specified high dynamic range image
    to a file in the RGBE format of the Radiance system (.pic or .hdr files),
    keeping the unbounded range of its components (see ImagePersistenceRgbe).
    @param fd
    @param img
    @return true if export was successful, false otherwise
    */
    public static boolean exportRGBE(File fd, RGBImageFloatUncompressed img)
    {
        if ( img == null ) {
            return false;
        }
        try {
            FileOutputStream fos;
            fos = new FileOutputStream(fd);
            BufferedOutputStream writer;
            writer = new BufferedOutputStream(fos);
            boolean status = exportRGBE(writer, img);
            writer.flush();
            writer.close();
            fos.close();
            return status;
        }
        catch (Exception e) {
            return false;
        }
    }

    /**
    Same as exportRGBE over a file, but writing on the given stream, which is
    not closed.
    @param os
    @param img
    @return true if export was successful, false otherwise
    */
    public static boolean exportRGBE(OutputStream os, RGBImageFloatUncompressed img)
    {
        try {
            return ImagePersistenceRgbe.exportRgbe(os, img);
        }
        catch (Exception e) {
            return false;
        }
    }

    /**
    Loads a high dynamic range image from a file in the RGBE format of the
    Radiance system (.pic or .hdr files).
    @param inImageFd - The file containing the image
    @return the image, or null if the file could not be read as an RGBE image
    */
    public static RGBImageFloatUncompressed importRGBE(File inImageFd)
    {
        String rgbeImportName = inImageFd.getPath();
        if ( !inImageFd.exists() ) {
            Logger.reportMessage(null, VSDK.ERROR, "importRGBE",
                "Cannot open RGBE file " + rgbeImportName);
            return null;
        }
        RGBImageFloatUncompressed image = null;
        try ( BufferedInputStream reader = new BufferedInputStream(new FileInputStream(inImageFd)) ) {
            image = ImagePersistenceRgbe.importRgbe(reader);
        }
        catch (Exception e) {
            image = null;
        }
        if ( image == null ) {
            Logger.reportMessage(null, VSDK.ERROR, "importRGBE",
                "Not a readable RGBE image: " + rgbeImportName);
        }
        return image;
    }

    /**
    This method writes the contents of the specified image to a file in
    binary RGB uncompressed BMP format. Returns true if everything
    works fine, false if something fails, like a permission access denied
    or if storage device runs out of space.
    @param fd
    @param img
    @return
    */
    public static boolean exportBMP(File fd, Image img)
    {
        try {
            BufferedOutputStream writer;
            FileOutputStream fos;
            fos = new FileOutputStream(fd);
            writer = new BufferedOutputStream(fos);

            int y, x;

            //- Write BMP header ----------------------------------------------
            byte magic[] = new byte[2];
            magic[0] = 'B';
            magic[1] = 'M';

            writeBytes(writer, magic);
            writeLongLE(writer, img.getXSize()*img.getYSize()*3 + 54);
            writeLongLE(writer, 0);
            writeLongLE(writer, 54);

            //- Write Windows V3 DIB header -----------------------------------
            writeLongLE(writer, 40);
            writeLongLE(writer, img.getXSize());
            writeLongLE(writer, img.getYSize());
            writeSignedShortLE(writer, 1);
            writeSignedShortLE(writer, 24);
            writeLongLE(writer, 0);
            writeLongLE(writer, 16);
            writeLongLE(writer, 2835);
            writeLongLE(writer, 2835);
            writeLongLE(writer, 0);
            writeLongLE(writer, 0);

            //- Manejo de imagenes nativas de 24 bits por pixel ---------------
            RGBPixel pixel;

            for ( y = img.getYSize() - 1; y >= 0; y-- ) {
                for ( x = 0; x < img.getXSize() ; x++ ) {
                    pixel = img.getPixelRgb(x, y);
                    writer.write(pixel.b);
                    writer.write(pixel.g);
                    writer.write(pixel.r);
                }
            }

            writer.flush();
            writer.close();
            fos.close();
        }
        catch (Exception e) {
            return false;
        }
        return true;
    }

    /**
    This method writes the contents of the specified image to a file in
    binary GrayScale PPM format (i.e. P5 PPM sub-format). Returns true if
    everything works fine, false if something fails, like a permission access
    denied or if storage device runs out of space.
    @param fd
    @param img
    @return
    */
    public static boolean exportPNM(File fd, Image img)
    {
        try {
            BufferedOutputStream writer;
            FileOutputStream fos;
            fos = new FileOutputStream(fd);
            writer = new BufferedOutputStream(fos);

            String line1 = "P5\n";
            String line2 = "# Image generated by VitralSDK (https://github.com/OscarChavarro/vitral)\n";
            String line3 = img.getXSize() + " " + img.getYSize() + "\n";
            String line4 = "255\n";
            byte arr[];

            arr = line1.getBytes();
            writer.write(arr, 0, arr.length);
            arr = line2.getBytes();
            writer.write(arr, 0, arr.length);
            arr = line3.getBytes();
            writer.write(arr, 0, arr.length);
            arr = line4.getBytes();
            writer.write(arr, 0, arr.length);

            int x;
            int y;
            for ( y = 0; y < img.getYSize(); y++ ) {
                for ( x = 0; x < img.getXSize(); x++ ) {
                    writer.write(img.getPixel8bitGrayScale(x, y));
                }
            }

            writer.flush();
            writer.close();
            fos.close();
        }
        catch (Exception e) {
            return false;
        }
        return true;
    }

}
