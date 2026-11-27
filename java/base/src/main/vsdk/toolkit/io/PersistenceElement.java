package vsdk.toolkit.io;

// Java basic classes
import java.io.File;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.StringTokenizer;

// VSDK Classes
import vsdk.toolkit.common.VSDK;

/**
A `PersistenceElement` in VitralSDK is a software element with
algorithms and data structures (i.e. a class) with the specific functionality
of providing persistence operations for a data Entity.

The PersistenceElement abstract class provides an interface for *Persistence
style classes. This serves three purposes:
  - To help in design level organization of persistence classes (this eases the
    study of the class hierarchy)
  - To provide a place to locate operations common to all persistence classes 
    and persistence private utility/supporting classes.  In particular, this 
    class contains basic low level persistence operations for converting bit 
    streams from and to basic numeric data types. Note that this code is NOT 
    portable, as it needs explicit programmer configuration for little-endian 
    or big-endian hardware platform (programmer should take care about how
    to configure attribute bigEndianArchitecture).
  - To provide means of accessing some operating system's native library
    files and other basic file system management.

Note that there are several methods used to handle byte arrays and change 
between little endian and bit endian orders. When the copies are done on
the same order (from little endian to little endian or from big endian to
big endian) the "Direct" versions are used. When copies are done on the
reverse order (from little endian to big endian or from big endian to
little endian) the "Invert" versions are used.
*/
public abstract class PersistenceElement {

    private static final boolean bigEndianArchitecture = false;

    // Those are not thread safe / re-entrant... each different thread should
    // use its own arrays
    private static final byte byteBuffer1byte[] = new byte[1];
    private static final byte byteBuffer2byte[] = new byte[2];
    private static final byte byteBuffer4byte[] = new byte[4];
    private static final byte byteBuffer8byte[] = new byte[8];
    
    // Long int should use an 8-sized array, not a 4-sized. Check.
    private static final byte[] bytesForLong = new byte[4];

    public static int
    readByteInt(InputStream is) throws Exception
    {
        int a;

        is.read(byteBuffer1byte, 0, 1);
        a = (int)byteBuffer1byte[0];

        return a;
    }

    public static int
    readByteUnsignedInt(InputStream is) throws Exception
    {
        int a;

        is.read(byteBuffer1byte, 0, 1);
        a = VSDK.signedByte2unsignedInteger(byteBuffer1byte[0]);

        return a;
    }

    /**
    Given a previously initialized array of bytes, this method fills it
    with information readed from the given input stream.  If it is not
    enough information to read, this method generates an Exception.
    @param is
    @param bytesBuffer
    @throws java.lang.Exception
    */
    public static void
    readBytes(InputStream is, byte[] bytesBuffer) throws Exception
    {
        int offset = 0;
        int numRead;
        int length = bytesBuffer.length;
        do {
            numRead = is.read(bytesBuffer, 
                              offset, (length-offset));
            offset += numRead;
        } while( offset < length && numRead >= 0 ); 
    }
    
    /**
    Given a previously initialized array of bytes, this method writes it
    with information readed from the given output stream.  If it is not
    enough information to read, this method generates an Exception.
    @param os
    @param bytesBuffer
    @throws java.lang.Exception
    */
    public static void
    writeBytes(OutputStream os, byte[] bytesBuffer) throws Exception
    {
        os.write(bytesBuffer, 0, bytesBuffer.length);
    }

    /**
    Receives an signed 16 bits integer (C++ short) and exports its data to a 
    signed 8 bit byte array in direct endianess order.
    @param outArrayToBeExported     byte array to be exported
    @param inStartIndexInsideArray  index inside byte array to export data from
    @param inNumberToConvert        signed 16 bits integer
    
    Pending to check: verify if inNumberToConvert parameter could be used
    of short type.
    */
    private static void signedShort2byteArrayDirect(
        byte outArrayToBeExported[], 
        final int inStartIndexInsideArray, 
        final int inNumberToConvert)
    {
        int i;
        int length = 2;

        // Convert number to array
        for ( i = 0; i < length; i++ ) {
            byteBuffer2byte[i] = 
                (byte)((inNumberToConvert & (0xFF << 8*i)) >> 8*i);
        }

        // Export subarray to end array
        int cnt;
        for ( i = inStartIndexInsideArray, cnt = 0; 
              i < (inStartIndexInsideArray + length); 
              i++, cnt++ ) {
            outArrayToBeExported[i] = byteBuffer2byte[cnt];
        }
    }

    /**
    Receives an signed 16 bits integer (C++ short) and exports its data to a 
    signed 8 bit byte array in reverse endianess order.
    @param outArrayToBeExported     byte array to be exported
    @param inStartIndexInsideArray  index inside byte array to export data from
    @param inNumberToConvert        signed 16 bits integer
    
    Pending to check: verify if inNumberToConvert parameter could be used
    of short type.
    */
    private static void signedShort2byteArrayInvert(
        byte outArrayToBeExported[], 
        int inStartIndexInsideArray, 
        int inNumberToConvert)
    {
        int i;
        int lenght = 2;

        // Convert number to array
        for ( i = 0; i < lenght; i++ ) {
            byteBuffer2byte[lenght-i-1] = 
                (byte)((inNumberToConvert & (0xFF << 8*i)) >> 8*i);
        }

        // Export subarray to end array
        int cnt;
        for ( i = inStartIndexInsideArray, cnt = 0; 
            i < (inStartIndexInsideArray + lenght); 
            i++, cnt++ ) {
            outArrayToBeExported[i] = byteBuffer2byte[cnt];
        }
    }

    private static int byteArray2signedShortDirect(byte[] arr, int start) {
        int low = arr[start] & 0xff;
        int high = arr[start+1] & 0xff;
        return ( high << 8 | low );
    }

    private static int byteArray2signedShortInvert(byte[] arr, int start) {
        int low = arr[start] & 0xff;
        int high = arr[start+1] & 0xff;
        return ( low << 8 | high );
    }

    private static long byteArray2longDirect(byte[] arr, int start) {
        int i;
        int len = 4;
        int cnt = 0;
        byte[] tmp = new byte[len];
        for ( i = start; i < (start + len); i++ ) {
            tmp[cnt] = arr[i];
            cnt++;
        }
        long accum = 0;
        i = 0;
        for ( int shiftBy = 0; shiftBy < 32; shiftBy += 8 ) {
            accum |= ( (long)( tmp[i] & 0xff ) ) << shiftBy;
            i++;
        }
        return accum;
    }

    private static void signedInt2byteArrayDirect(
        byte arr[],
        int start, 
        long num) {
        
        int i;
        int len = 4;
        byte[] tmp = new byte[len];

        // Convert number to array
        for ( i = 0; i < len; i++ ) {
            tmp[i] = (byte)((num & (0xFF << 8*i)) >> 8*i);
        }

        // Export subarray to end array
        int cnt;
        for ( i = start, cnt = 0; i < (start + len); i++, cnt++ ) {
            arr[i] = tmp[cnt];
        }
    }

    private static void signedInt2byteArrayInvert(
        byte[] arr, int start, long num) {
        int i;
        int len = 4;
        byte[] tmp = new byte[len];

        // Convert number to array
        for ( i = 0; i < len; i++ ) {
            tmp[len-i-1] = (byte)((num & (0xFF << 8*i)) >> 8*i);
        }

        // Export subarray to end array
        int cnt;
        for ( i = start, cnt = 0; i < (start + len); i++, cnt++ ) {
            arr[i] = tmp[cnt];
        }
    }

    private static long byteArray2longInvert(byte[] arr, int start) {
        int i;
        int len = 4;
        int cnt = 3;
        byte[] tmp = new byte[len];
        for ( i = start; i < (start + len); i++ ) {
            tmp[cnt] = arr[i];
            cnt--;
        }
        long accum = 0;
        i = 0;
        for ( int shiftBy = 0; shiftBy < 32; shiftBy += 8 ) {
            accum |= ( (long)( tmp[i] & 0xff ) ) << shiftBy;
            i++;
        }
        return accum;
    }

    private static float byteArray2floatDirect(byte[] arr, int start) {
        int i;
        int len = 4;
        int cnt;
        byte[] tmp = new byte[len];

        for ( i = start, cnt = 0; i < (start + len); i++, cnt++ ) {
            tmp[cnt] = arr[i];
        }
        int accum = 0;
        i = 0;
        for ( int shiftBy = 0; shiftBy < 32; shiftBy += 8 ) {
            accum |= ( (long)( tmp[i] & 0xff ) ) << shiftBy;
            i++;
        }
        return Float.intBitsToFloat(accum);
    }

    private static double byteArray2doubleDirect(byte[] arr, int start) {
        int i;
        int len = 8;
        int cnt;
        byte[] tmp = new byte[len];

        for ( i = start, cnt = 0; i < (start + len); i++, cnt++ ) {
            tmp[cnt] = arr[i];
        }
        long accum = 0;
        i = 0;
        for ( int shiftBy = 0; shiftBy < 64; shiftBy += 8 ) {
            accum |= ( (long)( tmp[i] & 0xff ) ) << shiftBy;
            i++;
        }
        return Double.longBitsToDouble(accum);
    }

    private static float byteArray2floatInvert(byte[] arr, int start) {
        int i;
        int len = 4;
        int cnt = 3;
        byte[] tmp = new byte[len];
        for ( i = start; i < (start + len); i++ ) {
            tmp[cnt] = arr[i];
            cnt--;
        }
        int accum = 0;
        i = 0;
        for ( int shiftBy = 0; shiftBy < 32; shiftBy += 8 ) {
            accum |= ( (long)( tmp[i] & 0xff ) ) << shiftBy;
            i++;
        }
        return Float.intBitsToFloat(accum);
    }

    private static double byteArray2doubleInvert(byte[] arr, int start) {
        int i;
        int len = 8;
        int cnt = 3;
        byte[] tmp = new byte[len];
        for ( i = start; i < (start + len); i++ ) {
            tmp[cnt] = arr[i];
            cnt--;
        }
        long accum = 0;
        i = 0;
        for ( int shiftBy = 0; shiftBy < 64; shiftBy += 8 ) {
            accum |= ( (long)( tmp[i] & 0xff ) ) << shiftBy;
            i++;
        }
        return Double.longBitsToDouble(accum);
    }

    /**
    This method is responsible of taking into account the endianess of the 
    original data
    @param arr
    @param start
    @return integer representation for given bit stream on big endian order
    */
    public static int byteArray2signedShortBE(byte arr[], int start) {
        if ( bigEndianArchitecture ) {
            return byteArray2signedShortDirect(arr, start);
        }
        return byteArray2signedShortInvert(arr, start);
    }

    public static void signedShort2byteArrayBE(byte[] arr, int start, int num) {
        if ( bigEndianArchitecture ) {
            signedShort2byteArrayDirect(arr, start, num);
        }
        signedShort2byteArrayInvert(arr, start, num);
    }

    public static void signedShort2byteArrayLE(byte[] arr, int start, int num) {
        if ( bigEndianArchitecture ) {
            signedShort2byteArrayInvert(arr, start, num);
        }
        signedShort2byteArrayDirect(arr, start, num);
    }

    /**
    This method is responsible of taking into account the endianess of the 
    original data
    @param arr
    @param start
    @return integer representation for given bit stream on little endian order
    */
    public static int byteArray2signedShortLE(byte[] arr, int start) {
        if ( bigEndianArchitecture ) {
            return byteArray2signedShortInvert(arr, start);
        }
        return byteArray2signedShortDirect(arr, start);
    }


    /**
    This method is responsible of taking into account the endianess of the 
    original data
    @param arr
    @param start
    @return long integer representation for given bit stream on big endian order
    */
    public static long byteArray2longBE(byte[] arr, int start) {
        if ( bigEndianArchitecture ) {
            return byteArray2longDirect(arr, start);
        }
        return byteArray2longInvert(arr, start);
    }

    /**
    This method is responsible of taking into account the endianess of the 
    original data
    @param arr
    @param start
    @return long integer representation for given bit stream on little
    endian order
    */
    public static long byteArray2longLE(byte[] arr, int start) {
        if ( bigEndianArchitecture ) {
            return byteArray2longInvert(arr, start);
        }
        return byteArray2longDirect(arr, start);
    }

    /**
    This method is responsible of taking into account the endianess of the 
    original data
    @param arr
    @param start
    @return single precision float number representation for given bit stream
    on big endian order
    */
    public static float byteArray2floatBE(byte[] arr, int start) {
        if ( bigEndianArchitecture ) {
            return byteArray2longDirect(arr, start);
        }
        return byteArray2longInvert(arr, start);
    }

    public static void float2byteArrayBE(byte[] arr, int start, float num) {
        long a = Float.floatToIntBits(num);
        if ( bigEndianArchitecture ) {
            signedInt2byteArrayDirect(arr, start, a);
        }
        signedInt2byteArrayInvert(arr, start, a);
    }

    public static void float2byteArrayLE(byte[] arr, int start, float num) {
        long a = Float.floatToIntBits(num);
        if ( bigEndianArchitecture ) {
            signedInt2byteArrayInvert(arr, start, a);
        }
        signedInt2byteArrayDirect(arr, start, a);
    }

    /**
    This method is responsible of taking into account the endianess of the 
    original data
    @param arr
    @param start
    @return single precision float representation for given bit stream on 
    little endian order
    */
    public static float byteArray2floatLE(byte[] arr, int start) {
        if ( bigEndianArchitecture ) {
            return byteArray2floatInvert(arr, start);
        }
        return byteArray2floatDirect(arr, start);
    }

    /**
    This method is responsible of taking into account the endianess of the 
    original data
    @param arr
    @param start
    @return double precission float representation for given bit stream on 
    little endian order
    */
    public static double byteArray2doubleLE(byte[] arr, int start) {
        if ( bigEndianArchitecture ) {
            return byteArray2doubleInvert(arr, start);
        }
        return byteArray2doubleDirect(arr, start);
    }
    
    /**
    @param arr    
    @param start    
    @return double precission float representation for given bit stream on 
    bit endian order
    */
    public static double byteArray2doubleBE(byte arr[], int start) {
        if ( bigEndianArchitecture ) {
            return byteArray2doubleDirect(arr, start);
        }
        return byteArray2doubleInvert(arr, start);
    }

    public static int readSignedShortLE(InputStream is) throws Exception
    {
        readBytes(is, byteBuffer2byte);
        return byteArray2signedShortLE(byteBuffer2byte, 0);
    }

    public static int readSignedShortBE(InputStream is) throws Exception
    {
        byte arr[] = new byte[2];
        readBytes(is, arr);
        return byteArray2signedShortBE(arr, 0);
    }

    public static void writeSignedShortBE(OutputStream os, int num) throws Exception
    {
        signedShort2byteArrayBE(byteBuffer2byte, 0, num);
        writeBytes(os, byteBuffer2byte);
    }

    public static void writeSignedShortLE(OutputStream os, int num) throws Exception
    {
        signedShort2byteArrayLE(byteBuffer2byte, 0, num);
        writeBytes(os, byteBuffer2byte);
    }

    /**
    Pending to check. Is this really managing 64 bit long integers?
    @param is
    @return 
    @throws java.lang.Exception
    */
    public static long readLongLE(InputStream is) throws Exception
    {
        readBytes(is, bytesForLong);
        return byteArray2longLE(bytesForLong, 0);
    }

    /**
    Pending to check. Is this really managing 64 bit long integers?
    @param is
    @return 
    @throws java.lang.Exception
    */
    public static long readLongBE(InputStream is) throws Exception
    {
        readBytes(is, bytesForLong);
        return byteArray2longBE(bytesForLong, 0);
    }
    
    public static float readFloatLE(InputStream is) throws Exception
    {
        readBytes(is, byteBuffer4byte);
        return byteArray2floatLE(byteBuffer4byte, 0);
    }

    public static double readDoubleLE(InputStream is) throws Exception
    {
        readBytes(is, byteBuffer8byte);
        return byteArray2doubleLE(byteBuffer8byte, 0);
    }

    public static double readDoubleBE(InputStream is) throws Exception
    {
        readBytes(is, byteBuffer8byte);
        return byteArray2doubleBE(byteBuffer8byte, 0);
    }

    public static float readFloatBE(InputStream is) throws Exception
    {
        readBytes(is, byteBuffer4byte);
        long i = byteArray2longBE(byteBuffer4byte, 0);
        int j = (int)i;
        return Float.intBitsToFloat(j);
    }

    public static void writeFloatBE(OutputStream os, float num)
        throws Exception
    {
        float2byteArrayBE(byteBuffer4byte, 0, num);
        writeBytes(os, byteBuffer4byte);
    }

    public static void writeFloatLE(OutputStream os, float num)
        throws Exception
    {
        float2byteArrayLE(byteBuffer4byte, 0, num);
        writeBytes(os, byteBuffer4byte);
    }

    public static void writeLongBE(OutputStream os, long num)
        throws Exception
    {
        if ( bigEndianArchitecture ) {
            signedInt2byteArrayDirect(bytesForLong, 0, num);
        }
        signedInt2byteArrayInvert(bytesForLong, 0, num);
        writeBytes(os, bytesForLong);
    }

    public static void writeLongLE(OutputStream os, long num)
        throws Exception
    {
        if ( bigEndianArchitecture ) {
            signedInt2byteArrayInvert(bytesForLong, 0, num);
        }
        signedInt2byteArrayDirect(bytesForLong, 0, num);
        writeBytes(os, bytesForLong);
    }

    public static String
    readAsciiFixedSizeString(InputStream is, int size) throws Exception
    {
        if ( size <= 0 ) {
            return "";
    	}
        
        // Old implementation: check if it is more ineficient and-or
        // equivalent
        /*
        byte bytesForString[] = new byte[size+1];
        readBytes(is, bytesForString);

        String msg = "";
        
        char letter;
        int i;

        for ( i = 0; i < size && bytesForString[i] != 0x00; i++ ) {
            letter = (char)bytesForString[i];
            if ( letter != 0x00 ) {
                msg = msg + letter;
            }
        }
        */

        // Alternative implementation by Leidy Lozano:        
        byte bytesForString[] = new byte[size];
        readBytes(is, bytesForString);
        
        String msg = new String(bytesForString, "UTF-8");
        
        byte skip[] = new byte[1];
        readBytes(is, skip);
        
        return msg;
        
    }

    public static String readAsciiString(InputStream is) throws Exception
    {
        byte character[] = new byte[1];
        char letter;
        StringBuilder msg = new StringBuilder("");

        do {
            readBytes(is, character);
            letter = (char)character[0];
            if ( character[0] != 0x00 ) {
                msg.append(letter);
            }
        } while ( character[0] != 0x00 );

        return msg.toString();
    }

    public static String readUtf8String(InputStream is) throws Exception
    {
        byte character[] = new byte[1];
        char letter;
        String msg = "";
        byte a[] = new byte[2];

        do {
            readBytes(is, character);
            letter = (char)character[0];
            if ( character[0] != 0x00 && ((letter >> 7) == 0) ) {
                msg = msg + letter;
            }
            else if ( character[0] != 0x00 ) {
                a[0] = character[0];
                if ( is.available() >= 1 ) {
                    readBytes(is, character);
                    a[1] = character[0];
                    String cc;
                    cc = buildUtf8Char(a);
                    if ( cc != null ) {
                        msg += cc;
                    }
                    else {
                        System.out.println("* UNHANDLED UTF! ********************************************************** ->" + msg);
                    }
                }
            }
        } while ( character[0] != 0x00 );

        return msg;
    }

    public static String buildUtf8Char(byte arr[])
    {
        String c;
        int a = VSDK.signedByte2unsignedInteger(arr[0]);
        int b = VSDK.signedByte2unsignedInteger(arr[1]);

        if ( ((a >> 5) == 0x06) &&
             ((b >> 6) == 0x02) ) {
            c = new String(arr);
        }
        else {
            //System.err.println("VSDK: Unhandled UTF-8 binary encoding!");
            //System.err.println("  - Byte 0: " + a + " / " + (a >> 5) );
            //System.err.println("  - Byte 1: " + b + " / " + (b >> 6) );
            return null;
        }
        return c;
    }

    public static String readUtf8Line(InputStream is) throws Exception
    {
        byte character[] = new byte[1];
        char letter;
        StringBuilder msg = new StringBuilder("");
        byte a[] = new byte[2];

        do {
            if ( is.available() < 1 ) return "";
            readBytes(is, character);
            letter = (char)character[0];
            if ( character[0] != '\n' && character[0] != '\r' &&
                 ((letter >> 7) == 0) ) {
                msg.append(letter);
            }
            else if ( character[0] != '\n' && character[0] != '\r' ) {
                a[0] = character[0];
                if ( is.available() >= 1 ) {
                    readBytes(is, character);
                    a[1] = character[0];
                    String cc = buildUtf8Char(a);
                    if ( cc != null ) {
                        msg.append(cc);
                    }
                }
            }
        } while ( character[0] != '\n' );

        return msg.toString();
    }

    public static String readAsciiLine(InputStream is) throws Exception
    {
        byte character[] = new byte[1];
        char letter;
        StringBuilder stringBuffer = new StringBuilder("");

        do {
            // Warning: this line makes this goes to slow!
            //if ( is.available() < 1 ) return "";
            readBytes(is, character);
            letter = (char)character[0];
            if ( character[0] != '\n' && character[0] != '\r' ) {
                stringBuffer.append(letter);
            }
        } while ( character[0] != '\n' );

        return stringBuffer.toString();
    }

    private static boolean isInSet(byte key, byte set[])
    {
        int i;

        for ( i = 0; i < set.length; i++ ) {
            if ( key == set[i] ) {
                return true;
            }
        }
        return false;
    }

    public static String readAsciiToken(InputStream is, byte separators[]) throws Exception
    {
        byte character[] = new byte[1];
        char letter;
        String msg = "";
        int i;

        do {
            readBytes(is, character);
            if ( !isInSet(character[0], separators) ) {
                msg = msg + ((char)character[0]);
            }
        } while ( !isInSet(character[0], separators) );

        return msg;
    }

    public static void
    writeAsciiString(OutputStream writer, String cad)
        throws Exception
    {
        byte arr[];
        arr = cad.getBytes();
        writer.write(arr, 0, arr.length);

        byte end[] = new byte[1];
        end[0] = '\0';
        writer.write(end, 0, end.length);
    }

    public static void
    writeUtf8String(OutputStream writer, String cad) throws Exception
    {
        byte arr[];
        arr = cad.getBytes();
        writer.write(arr, 0, arr.length);

        byte end[] = new byte[1];
        end[0] = '\0';
        writer.write(end, 0, end.length);
    }

    public static void
    writeAsciiLine(OutputStream writer, String cad)
        throws Exception
    {
        byte arr[];
        arr = cad.getBytes();
        writer.write(arr, 0, arr.length);

        byte end[] = new byte[1];
        end[0] = '\n';
        writer.write(end, 0, end.length);
    }

    public static void
    writeUtf8Line(OutputStream writer, String cad)
        throws Exception
    {
        byte arr[];
        arr = cad.getBytes();
        writer.write(arr, 0, arr.length);

        byte end[] = new byte[1];
        end[0] = '\n';
        writer.write(end, 0, end.length);
    }

    /**
    Given the name of a native library, this method tries to determine
    whether it is available or not.  Takes into account the cross-platform
    differences, and it is supposed to check if a System.loadLibrary
    call for given library will succeed or not.

    Use this method to anticipate any problem before it fails, so a better
    user feedback instruction can be given instead of waiting for an exception
    to be thrown.  Some libraries, as JOGL fails to return to the application
    the exception of a failed System.loadLibrary, so this method is useful
    in bettering the user feedback for this kind of circumstance.
    @param libname
    @return true if library is available
    */
    public static boolean verifyLibrary(String libname) {
        String nativeLibname = System.mapLibraryName(libname);
        String paths = System.getProperty("java.library.path");
        String os = System.getProperty("os.name").toLowerCase();

        if ( os.startsWith("linux") || os.startsWith("solaris") ||
             os.startsWith("unix") ) {
            paths = paths.concat(":/lib");
            paths = paths.concat(":/usr/lib");
            paths = paths.concat(":/usr/local/lib");
            paths = paths.concat(":/usr/X11R6/lib");
            paths = paths.concat(":/usr/X11R6/lib64");
            paths = paths.concat(":/usr/openwin/lib");
            paths = paths.concat(":/usr/dt/lib");
            paths = paths.concat(":/lib64");
            paths = paths.concat(":/usr/lib64");
            paths = paths.concat(":/usr/local/lib64");
            paths = paths.concat(":" + System.getenv("LD_LIBRARY_PATH"));
        }

        String separator = File.pathSeparator;                
        StringTokenizer st = new StringTokenizer(paths, separator);
        String token;
        String concat = File.separator;
        while ( st.hasMoreTokens() ) {
            token = st.nextToken();
            File directory = new File(token);
            if ( !directory.isDirectory()  ) {
                continue;
            }
            File file = new File(token + concat + nativeLibname);
            if ( file.exists() ) {
                return true;
            }
                        
        }
        return false;
    }

    public static boolean
    checkDirectory(String dirName)
    {
        File dirFd = new File(dirName);

        if ( dirFd.exists() && (!dirFd.isDirectory() ) ) {
            System.err.println("Directory " + dirName + " can not be created, because a file with that name already exists (not overwriten).");
            return false;
        }

        if ( !dirFd.exists() && !dirFd.mkdir() ) {
            System.err.println("Directory " + dirName + " can not be created, check permisions and available free disk space.");
            return false;
        }

        return true;
    }

    /**
    Given a filename, this method extract its extension and return it.
    \todo : This method will fail when directory path or filename contains
    more than one dot.  Needs to be fixed.
    @param fd
    @return file extension
    */
    protected static String extractExtensionFromFile(File fd)
    {
        String filename = fd.getName();
        StringTokenizer st = new StringTokenizer(filename, ".");
        int numTokens = st.countTokens();
        for( int i = 0; i < numTokens - 1; i++ ) {
            st.nextToken();
        }
        String ext = st.nextToken();
        return ext;
    }

}
