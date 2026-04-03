package vsdk.toolkit.common;

public final class CppReAlloc {
    private CppReAlloc() {
    }

    public static byte[] reAlloc(byte[] ptr, int oldElementCount, int newElementCount) {
        if (newElementCount <= 0) {
            return null;
        }

        byte[] newPtr = new byte[newElementCount];
        if (ptr != null && oldElementCount > 0) {
            int copyElementCount = Math.min(oldElementCount, newElementCount);
            System.arraycopy(ptr, 0, newPtr, 0, copyElementCount);
        }
        return newPtr;
    }

    public static char[][] reAlloc(char[][] ptr, int oldElementCount, int newElementCount) {
        if (newElementCount <= 0) {
            return null;
        }

        char[][] newPtr = new char[newElementCount][];
        if (ptr != null && oldElementCount > 0) {
            int copyElementCount = Math.min(oldElementCount, newElementCount);
            System.arraycopy(ptr, 0, newPtr, 0, copyElementCount);
        }
        return newPtr;
    }
}
