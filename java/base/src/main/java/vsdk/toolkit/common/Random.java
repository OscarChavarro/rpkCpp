package vsdk.toolkit.common;

import java.io.IOException;
import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodType;
import java.nio.file.Path;
import java.nio.file.Paths;

import jdk.incubator.foreign.CLinker;
import jdk.incubator.foreign.FunctionDescriptor;
import jdk.incubator.foreign.MemoryAccess;
import jdk.incubator.foreign.MemoryAddress;
import jdk.incubator.foreign.MemorySegment;
import jdk.incubator.foreign.ResourceScope;
import jdk.incubator.foreign.SymbolLookup;

import vsdk.toolkit.common.logging.Logger;

/**
 * Central 48-bit RNG used by the stochastic raytracing code. The C++ port
 * calls libc's drand48/srand48/seed48 directly, which are all backed by a
 * single process-global 48-bit LCG state. The stock java.util.Random uses a
 * different algorithm, so scenes rendered by the Java port produced
 * different pixels than the C++ port and failed the golden image tests.
 *
 * With nativeMode enabled (the default; see the app's -nativeRNG option to
 * turn it off), this class calls the very same libc functions through a
 * tiny native library (java/pkgs/nativeRandomNumberGenerator), bound with
 * the Java 17 incubator Foreign Function & Memory API (jdk.incubator.foreign),
 * so both ports draw from the same generator and produce bit-identical
 * sequences.
 *
 * When nativeMode is disabled (or the native library cannot be loaded),
 * a pure Java re-implementation of the same 48-bit LCG algorithm used by
 * glibc is used instead, so results still match on any platform.
 */
public final class Random {
    // Same constants glibc uses for drand48/srand48/seed48 (POSIX rand48 family)
    private static final long MULTIPLIER = 0x5DEECE66DL;
    private static final long ADDEND = 0xBL;
    private static final long MASK_48 = (1L << 48) - 1;

    private static boolean nativeMode = true;
    private static boolean started = false;

    // Pure Java fallback state (mirrors the C library's global 48-bit state)
    private static long javaState = 0x1234ABCDL;

    // Native bindings, resolved lazily on startup() when nativeMode is active
    private static ResourceScope nativeScope;
    private static MethodHandle nativeSrand48;
    private static MethodHandle nativeDrand48;
    private static MethodHandle nativeSeed48;
    private static MethodHandle nativeShutdown;

    private Random() {
    }

    public static synchronized void setNativeMode(boolean enabled) {
        if ( started ) {
            throw new IllegalStateException("Random.setNativeMode() must be called before startup()");
        }
        nativeMode = enabled;
    }

    public static synchronized boolean isNativeMode() {
        return nativeMode;
    }

    /**
     * Must be called once at application startup, before any rendering
     * happens, and matched by a single shutdown() call at the very end of
     * the application's execution.
     */
    public static synchronized void startup() {
        if ( started ) {
            return;
        }
        if ( nativeMode ) {
            try {
                bindNativeLibrary();
                nativeScope = ResourceScope.globalScope();
                nativeSrand48.invoke(0L);
            } catch ( Throwable error ) {
                Logger.warning("Random.startup", "could not load native RNG library, falling back to Java implementation: %s", error);
                nativeMode = false;
            }
        }
        started = true;
    }

    public static synchronized void shutdown() {
        if ( !started ) {
            return;
        }
        if ( nativeMode && nativeScope != null && nativeShutdown != null ) {
            try {
                nativeShutdown.invoke();
            } catch ( Throwable error ) {
                Logger.warning("Random.shutdown", "error during native RNG shutdown: %s", error);
            }
        }
        nativeSrand48 = null;
        nativeDrand48 = null;
        nativeSeed48 = null;
        nativeShutdown = null;
        nativeScope = null;
        started = false;
    }

    public static synchronized void srand48(long seed) {
        ensureStarted();
        if ( nativeMode ) {
            try {
                nativeSrand48.invoke(seed);
                return;
            } catch ( Throwable error ) {
                throw new RuntimeException("Random: native srand48 call failed", error);
            }
        }
        javaState = (seed << 16) & MASK_48 | 0x330EL;
    }

    public static synchronized double drand48() {
        ensureStarted();
        if ( nativeMode ) {
            try {
                return (double)nativeDrand48.invoke();
            } catch ( Throwable error ) {
                throw new RuntimeException("Random: native drand48 call failed", error);
            }
        }
        javaState = (MULTIPLIER * javaState + ADDEND) & MASK_48;
        return javaState / (double)(1L << 48);
    }

    /**
     * Sets the 48-bit generator state from newSeed[3] (each entry treated as
     * an unsigned 16-bit value) and returns the previous state, mirroring
     * the C library's seed48() semantics used by SeedConfig.
     */
    public static synchronized short[] seed48(short[] newSeed) {
        ensureStarted();
        if ( nativeMode ) {
            try (ResourceScope callScope = ResourceScope.newConfinedScope()) {
                MemorySegment in = MemorySegment.allocateNative(3L * Short.BYTES, callScope);
                MemorySegment out = MemorySegment.allocateNative(3L * Short.BYTES, callScope);
                for ( int i = 0; i < 3; i++ ) {
                    MemoryAccess.setShortAtOffset(in, (long)i * Short.BYTES, newSeed[i]);
                }
                nativeSeed48.invoke(in.address(), out.address());
                short[] result = new short[3];
                for ( int i = 0; i < 3; i++ ) {
                    result[i] = MemoryAccess.getShortAtOffset(out, (long)i * Short.BYTES);
                }
                return result;
            } catch ( Throwable error ) {
                throw new RuntimeException("Random: native seed48 call failed", error);
            }
        }
        short[] previous = javaStateToSeed();
        long merged = (((long)newSeed[0] & 0xFFFFL) << 32)
            | (((long)newSeed[1] & 0xFFFFL) << 16)
            | ((long)newSeed[2] & 0xFFFFL);
        javaState = merged & MASK_48;
        return previous;
    }

    private static short[] javaStateToSeed() {
        return new short[] {
            (short)((javaState >>> 32) & 0xFFFFL),
            (short)((javaState >>> 16) & 0xFFFFL),
            (short)(javaState & 0xFFFFL)
        };
    }

    private static void ensureStarted() {
        if ( !started ) {
            startup();
        }
    }

    private static void bindNativeLibrary() throws Throwable {
        Path libraryPath = resolveLibraryPath();
        System.load(libraryPath.toAbsolutePath().toString());

        SymbolLookup lookup = SymbolLookup.loaderLookup();
        nativeSrand48 = downcallFrom(lookup, "nativeSrand48", MethodType.methodType(void.class, long.class), FunctionDescriptor.ofVoid(CLinker.C_LONG));
        nativeDrand48 = downcallFrom(lookup, "nativeDrand48", MethodType.methodType(double.class), FunctionDescriptor.of(CLinker.C_DOUBLE));
        nativeSeed48 = downcallFrom(
            lookup,
            "nativeSeed48",
            MethodType.methodType(void.class, MemoryAddress.class, MemoryAddress.class),
            FunctionDescriptor.ofVoid(CLinker.C_POINTER, CLinker.C_POINTER));
        nativeShutdown = downcallFrom(lookup, "nativeRandomShutdown", MethodType.methodType(void.class), FunctionDescriptor.ofVoid());
        downcallFrom(lookup, "nativeRandomStartup", MethodType.methodType(void.class), FunctionDescriptor.ofVoid()).invoke();
    }

    private static MethodHandle downcallFrom(SymbolLookup lookup, String symbolName, MethodType type, FunctionDescriptor descriptor) {
        MemoryAddress address = lookup.lookup(symbolName)
            .orElseThrow(() -> new IllegalStateException("Symbol not found in native RNG library: " + symbolName));
        return CLinker.getInstance().downcallHandle(address, type, descriptor);
    }

    private static final String LIBRARY_FILE_NAME = "librpkNativeRandom.so";
    private static final String LIBRARY_PATH_PROPERTY = "rpk.native.random.library";

    // Gradle (see java/base/build.gradle) builds the native module with
    // CMake and copies the resulting shared library into
    // base/build/nativeLibs, relative to the java/ root project directory,
    // which is also the working directory the run/test tasks use. These
    // candidates cover running from java/, from java/base/, and from
    // inside an IDE working directory set to the module directory.
    private static final String[] CANDIDATE_RELATIVE_PATHS = {
        "base/build/nativeLibs/" + LIBRARY_FILE_NAME,
        "build/nativeLibs/" + LIBRARY_FILE_NAME,
        "../base/build/nativeLibs/" + LIBRARY_FILE_NAME,
        "java/base/build/nativeLibs/" + LIBRARY_FILE_NAME,
    };

    private static Path resolveLibraryPath() throws IOException {
        String explicitPath = System.getProperty(LIBRARY_PATH_PROPERTY);
        if ( explicitPath != null ) {
            Path path = Paths.get(explicitPath);
            if ( !path.toFile().isFile() ) {
                throw new IOException("Native RNG library not found at -D" + LIBRARY_PATH_PROPERTY + "=" + explicitPath);
            }
            return path;
        }
        for ( String candidate : CANDIDATE_RELATIVE_PATHS ) {
            Path path = Paths.get(candidate);
            if ( path.toFile().isFile() ) {
                return path;
            }
        }
        throw new IOException("Native RNG library (" + LIBRARY_FILE_NAME + ") not found. Build it via "
            + "java/pkgs/nativeRandomNumberGenerator (CMake) - the base module's Gradle build does this "
            + "automatically - or pass -D" + LIBRARY_PATH_PROPERTY + "=/path/to/" + LIBRARY_FILE_NAME + ".");
    }
}
