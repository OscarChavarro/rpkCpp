package vsdk.toolkit.io.image;

/**
Image geometry reported by the `NativeImageReader` native library, plus the
reference to the native reading context associated with it.

Note this class does not extend `PersistenceElement`: it is compiled stand
alone by the `pkgs/NativeImageReader` build to generate the JNI header, so
it must not depend on the rest of the toolkit.
*/
public class _NativeImageReaderWrapperHeaderInfo
{
    public long xSize;
    public long ySize;
    public long channels;

    /**
    Address of the native `_NativeImageReaderHeaderInfo` structure of the
    reading operation in progress, or 0 if no reading is pending. Only
    manipulated by the native library.
    */
    public long nativePointer;
}
