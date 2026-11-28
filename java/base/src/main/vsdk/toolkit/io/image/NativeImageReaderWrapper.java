package vsdk.toolkit.io.image;

// Basic JDK classes
import java.nio.ByteBuffer;

// VitralSDK classes
import vsdk.toolkit.common.NativeLibraryLoader;

/**
Java binding for the `NativeImageReader` native library (the CMake project
`pkgs/NativeImageReader`), a libpng based png reader which is much faster
than the AWT based fall back used by `ImagePersistence`.

The library is optional: if it can not be located or loaded, `available`
stays false and no exception is propagated to the application, which then
uses the pure Java reading path. See `vsdk.toolkit.common.NativeLibraryLoader`
for the places where the library is looked for.
*/
public class NativeImageReaderWrapper
{
    public static final String LIBRARY_NAME = "NativeImageReader";

    /**
    ABI version implemented by this class. The native library reports its
    own version through `getApiVersion`, so an obsolete library installed
    in a system directory is rejected instead of failing, much later, with
    an UnsatisfiedLinkError in the middle of an image reading.
    */
    public static final int REQUIRED_API_VERSION = 1;

    /**
    True if the native library was loaded and its methods can be called.
    */
    public static final boolean available;

    private static String loadedLibraryPath = null;
    private static String loadDiagnostic = null;

    static {
        available = loadNativeLibrary();
    }

    //= Library loading =====================================================

    private static boolean loadNativeLibrary()
    {
        String diagnostic[] = new String[1];
        String path = NativeLibraryLoader.load(LIBRARY_NAME, LIBRARY_NAME,
                                               diagnostic);

        loadDiagnostic = diagnostic[0];

        if ( path == null ) {
            return false;
        }

        try {
            int version = getApiVersion();

            if ( version == REQUIRED_API_VERSION ) {
                loadedLibraryPath = path;
                return true;
            }
            loadDiagnostic = path + ": unsupported library version " +
                version + " (" + REQUIRED_API_VERSION + " needed), rebuild " +
                "it with pkgs/NativeImageReader";
        }
        catch ( Throwable e ) {
            loadDiagnostic = path + ": obsolete library, rebuild it with " +
                "pkgs/NativeImageReader";
        }

        return false;
    }

    //= Diagnostics =========================================================

    /**
    Returns the path of the native library in use, or null if the library
    could not be loaded.
    */
    public static String getLoadedLibraryPath()
    {
        return loadedLibraryPath;
    }

    /**
    Returns the reason why the native library is not in use, or null if no
    loading attempt failed.
    */
    public static String getLoadDiagnostic()
    {
        return loadDiagnostic;
    }

    //= Native methods ======================================================

    /**
    Returns the ABI version implemented by the native library.
    */
    private static native int getApiVersion();

    /**
    Reads the header of the png image contained in `filename`, filling the
    image geometry in `header` and leaving an open native reading context
    referenced by it. The context is released by any of the
    `readPngData...` methods, or by `releasePngHeader`.
    */
    public static native void readPngHeader(_NativeImageReaderWrapperHeaderInfo header, String filename) throws Exception;

    /**
    Reads the pending image data of `header` as 8 bit RGB in to the direct
    buffer `arr`, and releases the native reading context.
    */
    public static native void readPngDataRGB(_NativeImageReaderWrapperHeaderInfo header, ByteBuffer arr) throws Exception;

    /**
    Reads the pending image data of `header` as 8 bit RGBA in to the direct
    buffer `arr`, and releases the native reading context.
    */
    public static native void readPngDataRGBA(_NativeImageReaderWrapperHeaderInfo header, ByteBuffer arr) throws Exception;

    /**
    Releases the native reading context of `header`, if any. Calling this
    method on an already released header has no effect.
    */
    public static native void releasePngHeader(_NativeImageReaderWrapperHeaderInfo header);
}
