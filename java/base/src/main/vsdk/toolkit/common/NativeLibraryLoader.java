package vsdk.toolkit.common;

// Basic JDK classes
import java.io.File;
import java.net.URL;
import java.util.ArrayList;

/**
Locates and loads the optional native libraries of the Vitral toolkit (the
CMake projects contained in the `pkgs` directory of the source tree).

The old mechanism depended on `-Djava.library.path` pointing to the place
where a `make install` left the libraries, so applications launched in any
other way reported the library as missing (or, worse, aborted with an
UnsatisfiedLinkError thrown from a static initialization block). This class
looks for the library in the source tree itself, so a freshly built library
is used without any additional configuration.

Search order:
  1. The file pointed by the `vsdk.nativeLibrary.<name>` system property.
  2. The directories listed in the `VSDK_NATIVE_LIB_PATH` environment variable.
  3. The build directory of the corresponding project inside the Vitral
     source tree (i.e. `pkgs/NativeImageReader/build`), as located from the
     current directory and from the place the toolkit was loaded from.
  4. `System.loadLibrary`, that is, the `java.library.path` directories.
  5. The usual system library directories.

Note this class does not extend any toolkit class: it is compiled stand
alone by the `pkgs` native projects in order to generate their JNI headers.
*/
public class NativeLibraryLoader
{
    private NativeLibraryLoader()
    {
    }

    /**
    Loads the native library `libraryName`, built by the `pkgs/packageName`
    project. Returns the path of the loaded library, or null if it can not
    be found or loaded: this method never throws, as all the native
    libraries of the toolkit are optional.

    @param libraryName - library name, as used by System.loadLibrary
    @param packageName - name of the pkgs project building the library
    @param diagnostic - if not null, a one element array where the reason
                        of the last failed attempt is reported
    */
    public static String load(String libraryName, String packageName,
                              String diagnostic[])
    {
        for ( File candidate : buildCandidateList(libraryName, packageName) ) {
            if ( !candidate.isFile() ) {
                continue;
            }

            String path = candidate.getAbsolutePath();

            try {
                System.load(path);
                return path;
            }
            catch ( Throwable e ) {
                reportDiagnostic(diagnostic, path + ": " + e.getMessage());
            }
        }

        try {
            System.loadLibrary(libraryName);
            return System.mapLibraryName(libraryName) +
                   " (from java.library.path)";
        }
        catch ( Throwable e ) {
            reportDiagnostic(diagnostic, e.getMessage());
        }

        return null;
    }

    private static void reportDiagnostic(String diagnostic[], String message)
    {
        if ( diagnostic != null && diagnostic.length > 0 ) {
            diagnostic[0] = message;
        }
    }

    /**
    Builds the ordered list of files where a native library could be, as
    documented in this class header.
    */
    private static ArrayList<File> buildCandidateList(String libraryName,
                                                      String packageName)
    {
        ArrayList<File> candidates = new ArrayList<File>();
        String nativeLibname = System.mapLibraryName(libraryName);

        String explicitLibrary =
            System.getProperty("vsdk.nativeLibrary." + libraryName);

        if ( explicitLibrary != null && !explicitLibrary.isEmpty() ) {
            candidates.add(new File(explicitLibrary));
        }

        ArrayList<String> directories = new ArrayList<String>();

        addPathList(directories, System.getenv("VSDK_NATIVE_LIB_PATH"));

        for ( File sourceTree : locateSourceTrees(packageName) ) {
            directories.add(new File(sourceTree,
                "pkgs" + File.separator + packageName +
                File.separator + "build").getPath());
            // Location used by the old Makefile based build
            directories.add(new File(sourceTree, "lib").getPath());
        }

        addPathList(directories, System.getProperty("java.library.path"));
        addPathList(directories, System.getenv("LD_LIBRARY_PATH"));
        addPathList(directories, System.getenv("DYLD_LIBRARY_PATH"));

        directories.add("/usr/local/lib");
        directories.add("/usr/lib");
        directories.add("/lib");
        // MacOSX package managers (Homebrew on Apple Silicon, MacPorts)
        directories.add("/opt/homebrew/lib");
        directories.add("/opt/local/lib");

        for ( String directory : directories ) {
            candidates.add(new File(directory, nativeLibname));
        }

        return candidates;
    }

    private static void addPathList(ArrayList<String> directories,
                                    String pathList)
    {
        if ( pathList == null || pathList.isEmpty() ) {
            return;
        }

        for ( String entry : pathList.split(File.pathSeparator) ) {
            if ( !entry.isEmpty() ) {
                directories.add(entry);
            }
        }
    }

    /**
    Returns the Vitral source tree roots reachable from the current
    directory and from the place this class was loaded from, so a library
    built inside the source tree is found however the application was
    launched. Only directories containing the `pkgs/packageName` project
    are returned.
    */
    private static ArrayList<File> locateSourceTrees(String packageName)
    {
        ArrayList<File> roots = new ArrayList<File>();

        addAncestors(roots, new File(System.getProperty("user.dir", ".")),
                     packageName);

        try {
            URL location = NativeLibraryLoader.class.getProtectionDomain().
                getCodeSource().getLocation();

            if ( location != null && "file".equals(location.getProtocol()) ) {
                addAncestors(roots, new File(location.toURI()), packageName);
            }
        }
        catch ( Throwable e ) {
            // The code source is not always available (i.e. with custom
            // class loaders). In that case, only the current directory is
            // used as a hint.
        }

        return roots;
    }

    private static void addAncestors(ArrayList<File> roots, File start,
                                     String packageName)
    {
        File current;

        try {
            current = start.getCanonicalFile();
        }
        catch ( Throwable e ) {
            current = start.getAbsoluteFile();
        }

        while ( current != null ) {
            if ( new File(current, "pkgs" + File.separator + packageName).
                     isDirectory() ) {
                roots.add(current);
            }
            current = current.getParentFile();
        }
    }
}
