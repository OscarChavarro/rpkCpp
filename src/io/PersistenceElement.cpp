#include "io/PersistenceElement.h"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <sys/stat.h>

#if defined(_WIN32)
#include <direct.h>
#endif

namespace vsdk {

const bool PersistenceElement::bigEndianArchitecture = false;
unsigned char PersistenceElement::byteBuffer1byte[1] = {0};
unsigned char PersistenceElement::byteBuffer2byte[2] = {0, 0};
unsigned char PersistenceElement::byteBuffer4byte[4] = {0, 0, 0, 0};
unsigned char PersistenceElement::byteBuffer8byte[8] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned char PersistenceElement::bytesForLong[4] = {0, 0, 0, 0};

static int
signedByte2unsignedInteger(unsigned char value) {
    return static_cast<int>(value);
}

int
PersistenceElement::readByteInt(java::io::InputStream &is) {
    readBytes(is, byteBuffer1byte, 1);
    return static_cast<int>(static_cast<signed char>(byteBuffer1byte[0]));
}

int
PersistenceElement::readByteUnsignedInt(java::io::InputStream &is) {
    readBytes(is, byteBuffer1byte, 1);
    return signedByte2unsignedInteger(byteBuffer1byte[0]);
}

/**
Given a previously initialized array of bytes, this method fills it
with information readed from the given input stream.  If it is not
enough information to read, this method generates an Exception.
@param is
@param bytesBuffer
*/
void
PersistenceElement::readBytes(java::io::InputStream &is, unsigned char *bytesBuffer, int length) {
    if ( bytesBuffer == nullptr || length < 0 ) {
        throw std::runtime_error("PersistenceElement::readBytes invalid buffer");
    }
    int offset = 0;
    int numRead = 0;
    do {
        numRead = is.read(bytesBuffer, offset, (length - offset));
        if ( numRead <= 0 ) {
            break;
        }
        offset += numRead;
    } while ( offset < length && numRead >= 0 );

    if ( offset < length ) {
        throw std::runtime_error("PersistenceElement::readBytes could not read requested length");
    }
}

/**
Given a previously initialized array of bytes, this method writes it
with information readed from the given output stream.  If it is not
enough information to read, this method generates an Exception.
@param os
@param bytesBuffer
*/
void
PersistenceElement::writeBytes(FILE *os, const unsigned char *bytesBuffer, int length) {
    if ( os == nullptr || bytesBuffer == nullptr || length < 0 ) {
        throw std::runtime_error("PersistenceElement::writeBytes invalid arguments");
    }
    const size_t written = fwrite(bytesBuffer, 1, static_cast<size_t>(length), os);
    if ( written != static_cast<size_t>(length) ) {
        throw std::runtime_error("PersistenceElement::writeBytes failed");
    }
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
void
PersistenceElement::signedShort2byteArrayDirect(
    unsigned char *outArrayToBeExported,
    const int inStartIndexInsideArray,
    const int inNumberToConvert)
{
    int i;
    const int length = 2;

    for ( i = 0; i < length; i++ ) {
        byteBuffer2byte[i] = static_cast<unsigned char>((inNumberToConvert & (0xFF << 8 * i)) >> (8 * i));
    }

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
void
PersistenceElement::signedShort2byteArrayInvert(
    unsigned char *outArrayToBeExported,
    int inStartIndexInsideArray,
    int inNumberToConvert)
{
    int i;
    const int length = 2;

    for ( i = 0; i < length; i++ ) {
        byteBuffer2byte[length - i - 1] = static_cast<unsigned char>((inNumberToConvert & (0xFF << 8 * i)) >> (8 * i));
    }

    int cnt;
    for ( i = inStartIndexInsideArray, cnt = 0;
          i < (inStartIndexInsideArray + length);
          i++, cnt++ ) {
        outArrayToBeExported[i] = byteBuffer2byte[cnt];
    }
}

int
PersistenceElement::byteArray2signedShortDirect(const unsigned char *arr, int start) {
    const int low = arr[start] & 0xff;
    const int high = arr[start + 1] & 0xff;
    return (high << 8 | low);
}

int
PersistenceElement::byteArray2signedShortInvert(const unsigned char *arr, int start) {
    const int low = arr[start] & 0xff;
    const int high = arr[start + 1] & 0xff;
    return (low << 8 | high);
}

long
PersistenceElement::byteArray2longDirect(const unsigned char *arr, int start) {
    int i;
    const int len = 4;
    int cnt = 0;
    unsigned char tmp[len];

    for ( i = start; i < (start + len); i++ ) {
        tmp[cnt] = arr[i];
        cnt++;
    }

    long accum = 0;
    i = 0;
    for ( int shiftBy = 0; shiftBy < 32; shiftBy += 8 ) {
        accum |= (static_cast<long>(tmp[i] & 0xff)) << shiftBy;
        i++;
    }
    return accum;
}

void
PersistenceElement::signedInt2byteArrayDirect(unsigned char *arr, int start, long num) {
    int i;
    const int len = 4;
    unsigned char tmp[len];

    for ( i = 0; i < len; i++ ) {
        tmp[i] = static_cast<unsigned char>((num & (0xFFL << (8 * i))) >> (8 * i));
    }

    int cnt;
    for ( i = start, cnt = 0; i < (start + len); i++, cnt++ ) {
        arr[i] = tmp[cnt];
    }
}

void
PersistenceElement::signedInt2byteArrayInvert(unsigned char *arr, int start, long num) {
    int i;
    const int len = 4;
    unsigned char tmp[len];

    for ( i = 0; i < len; i++ ) {
        tmp[len - i - 1] = static_cast<unsigned char>((num & (0xFFL << (8 * i))) >> (8 * i));
    }

    int cnt;
    for ( i = start, cnt = 0; i < (start + len); i++, cnt++ ) {
        arr[i] = tmp[cnt];
    }
}

long
PersistenceElement::byteArray2longInvert(const unsigned char *arr, int start) {
    int i;
    const int len = 4;
    int cnt = 3;
    unsigned char tmp[len];

    for ( i = start; i < (start + len); i++ ) {
        tmp[cnt] = arr[i];
        cnt--;
    }

    long accum = 0;
    i = 0;
    for ( int shiftBy = 0; shiftBy < 32; shiftBy += 8 ) {
        accum |= (static_cast<long>(tmp[i] & 0xff)) << shiftBy;
        i++;
    }
    return accum;
}

float
PersistenceElement::byteArray2floatDirect(const unsigned char *arr, int start) {
    int i;
    const int len = 4;
    int cnt;
    unsigned char tmp[len];

    for ( i = start, cnt = 0; i < (start + len); i++, cnt++ ) {
        tmp[cnt] = arr[i];
    }

    uint32_t accum = 0;
    i = 0;
    for ( int shiftBy = 0; shiftBy < 32; shiftBy += 8 ) {
        accum |= static_cast<uint32_t>(tmp[i] & 0xff) << shiftBy;
        i++;
    }

    float out = 0.0f;
    std::memcpy(&out, &accum, sizeof(float));
    return out;
}

double
PersistenceElement::byteArray2doubleDirect(const unsigned char *arr, int start) {
    int i;
    const int len = 8;
    int cnt;
    unsigned char tmp[len];

    for ( i = start, cnt = 0; i < (start + len); i++, cnt++ ) {
        tmp[cnt] = arr[i];
    }

    uint64_t accum = 0;
    i = 0;
    for ( int shiftBy = 0; shiftBy < 64; shiftBy += 8 ) {
        accum |= static_cast<uint64_t>(tmp[i] & 0xff) << shiftBy;
        i++;
    }

    double out = 0.0;
    std::memcpy(&out, &accum, sizeof(double));
    return out;
}

float
PersistenceElement::byteArray2floatInvert(const unsigned char *arr, int start) {
    int i;
    const int len = 4;
    int cnt = 3;
    unsigned char tmp[len];
    for ( i = start; i < (start + len); i++ ) {
        tmp[cnt] = arr[i];
        cnt--;
    }

    uint32_t accum = 0;
    i = 0;
    for ( int shiftBy = 0; shiftBy < 32; shiftBy += 8 ) {
        accum |= static_cast<uint32_t>(tmp[i] & 0xff) << shiftBy;
        i++;
    }

    float out = 0.0f;
    std::memcpy(&out, &accum, sizeof(float));
    return out;
}

double
PersistenceElement::byteArray2doubleInvert(const unsigned char *arr, int start) {
    int i;
    const int len = 8;
    int cnt = 7;
    unsigned char tmp[len];
    for ( i = start; i < (start + len); i++ ) {
        tmp[cnt] = arr[i];
        cnt--;
    }

    uint64_t accum = 0;
    i = 0;
    for ( int shiftBy = 0; shiftBy < 64; shiftBy += 8 ) {
        accum |= static_cast<uint64_t>(tmp[i] & 0xff) << shiftBy;
        i++;
    }

    double out = 0.0;
    std::memcpy(&out, &accum, sizeof(double));
    return out;
}

int
PersistenceElement::byteArray2signedShortBE(const unsigned char *arr, int start) {
    if ( bigEndianArchitecture ) {
        return byteArray2signedShortDirect(arr, start);
    }
    return byteArray2signedShortInvert(arr, start);
}

void
PersistenceElement::signedShort2byteArrayBE(unsigned char *arr, int start, int num) {
    if ( bigEndianArchitecture ) {
        signedShort2byteArrayDirect(arr, start, num);
    }
    signedShort2byteArrayInvert(arr, start, num);
}

void
PersistenceElement::signedShort2byteArrayLE(unsigned char *arr, int start, int num) {
    if ( bigEndianArchitecture ) {
        signedShort2byteArrayInvert(arr, start, num);
    }
    signedShort2byteArrayDirect(arr, start, num);
}

int
PersistenceElement::byteArray2signedShortLE(const unsigned char *arr, int start) {
    if ( bigEndianArchitecture ) {
        return byteArray2signedShortInvert(arr, start);
    }
    return byteArray2signedShortDirect(arr, start);
}

long
PersistenceElement::byteArray2longBE(const unsigned char *arr, int start) {
    if ( bigEndianArchitecture ) {
        return byteArray2longDirect(arr, start);
    }
    return byteArray2longInvert(arr, start);
}

long
PersistenceElement::byteArray2longLE(const unsigned char *arr, int start) {
    if ( bigEndianArchitecture ) {
        return byteArray2longInvert(arr, start);
    }
    return byteArray2longDirect(arr, start);
}

float
PersistenceElement::byteArray2floatBE(const unsigned char *arr, int start) {
    if ( bigEndianArchitecture ) {
        return static_cast<float>(byteArray2longDirect(arr, start));
    }
    return static_cast<float>(byteArray2longInvert(arr, start));
}

void
PersistenceElement::float2byteArrayBE(unsigned char *arr, int start, float num) {
    uint32_t bits = 0;
    std::memcpy(&bits, &num, sizeof(uint32_t));
    const long a = static_cast<long>(bits);
    if ( bigEndianArchitecture ) {
        signedInt2byteArrayDirect(arr, start, a);
    }
    signedInt2byteArrayInvert(arr, start, a);
}

void
PersistenceElement::float2byteArrayLE(unsigned char *arr, int start, float num) {
    uint32_t bits = 0;
    std::memcpy(&bits, &num, sizeof(uint32_t));
    const long a = static_cast<long>(bits);
    if ( bigEndianArchitecture ) {
        signedInt2byteArrayInvert(arr, start, a);
    }
    signedInt2byteArrayDirect(arr, start, a);
}

float
PersistenceElement::byteArray2floatLE(const unsigned char *arr, int start) {
    if ( bigEndianArchitecture ) {
        return byteArray2floatInvert(arr, start);
    }
    return byteArray2floatDirect(arr, start);
}

double
PersistenceElement::byteArray2doubleLE(const unsigned char *arr, int start) {
    if ( bigEndianArchitecture ) {
        return byteArray2doubleInvert(arr, start);
    }
    return byteArray2doubleDirect(arr, start);
}

double
PersistenceElement::byteArray2doubleBE(const unsigned char *arr, int start) {
    if ( bigEndianArchitecture ) {
        return byteArray2doubleDirect(arr, start);
    }
    return byteArray2doubleInvert(arr, start);
}

int
PersistenceElement::readSignedShortLE(java::io::InputStream &is) {
    readBytes(is, byteBuffer2byte, 2);
    return byteArray2signedShortLE(byteBuffer2byte, 0);
}

int
PersistenceElement::readSignedShortBE(java::io::InputStream &is) {
    unsigned char arr[2] = {0, 0};
    readBytes(is, arr, 2);
    return byteArray2signedShortBE(arr, 0);
}

void
PersistenceElement::writeSignedShortBE(FILE *os, int num) {
    signedShort2byteArrayBE(byteBuffer2byte, 0, num);
    writeBytes(os, byteBuffer2byte, 2);
}

void
PersistenceElement::writeSignedShortLE(FILE *os, int num) {
    signedShort2byteArrayLE(byteBuffer2byte, 0, num);
    writeBytes(os, byteBuffer2byte, 2);
}

/**
Pending to check. Is this really managing 64 bit long integers?
@param is
@return
*/
long
PersistenceElement::readLongLE(java::io::InputStream &is) {
    readBytes(is, bytesForLong, 4);
    return byteArray2longLE(bytesForLong, 0);
}

/**
Pending to check. Is this really managing 64 bit long integers?
@param is
@return
*/
long
PersistenceElement::readLongBE(java::io::InputStream &is) {
    readBytes(is, bytesForLong, 4);
    return byteArray2longBE(bytesForLong, 0);
}

float
PersistenceElement::readFloatLE(java::io::InputStream &is) {
    readBytes(is, byteBuffer4byte, 4);
    return byteArray2floatLE(byteBuffer4byte, 0);
}

double
PersistenceElement::readDoubleLE(java::io::InputStream &is) {
    readBytes(is, byteBuffer8byte, 8);
    return byteArray2doubleLE(byteBuffer8byte, 0);
}

double
PersistenceElement::readDoubleBE(java::io::InputStream &is) {
    readBytes(is, byteBuffer8byte, 8);
    return byteArray2doubleBE(byteBuffer8byte, 0);
}

float
PersistenceElement::readFloatBE(java::io::InputStream &is) {
    readBytes(is, byteBuffer4byte, 4);
    const long i = byteArray2longBE(byteBuffer4byte, 0);
    const uint32_t j = static_cast<uint32_t>(i);
    float out = 0.0f;
    std::memcpy(&out, &j, sizeof(float));
    return out;
}

void
PersistenceElement::writeFloatBE(FILE *os, float num) {
    float2byteArrayBE(byteBuffer4byte, 0, num);
    writeBytes(os, byteBuffer4byte, 4);
}

void
PersistenceElement::writeFloatLE(FILE *os, float num) {
    float2byteArrayLE(byteBuffer4byte, 0, num);
    writeBytes(os, byteBuffer4byte, 4);
}

void
PersistenceElement::writeLongBE(FILE *os, long num) {
    if ( bigEndianArchitecture ) {
        signedInt2byteArrayDirect(bytesForLong, 0, num);
    }
    signedInt2byteArrayInvert(bytesForLong, 0, num);
    writeBytes(os, bytesForLong, 4);
}

void
PersistenceElement::writeLongLE(FILE *os, long num) {
    if ( bigEndianArchitecture ) {
        signedInt2byteArrayInvert(bytesForLong, 0, num);
    }
    signedInt2byteArrayDirect(bytesForLong, 0, num);
    writeBytes(os, bytesForLong, 4);
}

std::string
PersistenceElement::readAsciiFixedSizeString(java::io::InputStream &is, int size) {
    if ( size <= 0 ) {
        return "";
    }

    std::vector<unsigned char> bytesForString(static_cast<size_t>(size));
    readBytes(is, bytesForString.data(), size);

    std::string msg(reinterpret_cast<const char *>(bytesForString.data()), bytesForString.size());

    unsigned char skip[1] = {0};
    readBytes(is, skip, 1);

    return msg;
}

std::string
PersistenceElement::readAsciiString(java::io::InputStream &is) {
    unsigned char character[1] = {0};
    std::string msg;

    do {
        readBytes(is, character, 1);
        if ( character[0] != 0x00 ) {
            msg.push_back(static_cast<char>(character[0]));
        }
    } while ( character[0] != 0x00 );

    return msg;
}

std::string
PersistenceElement::readUtf8String(java::io::InputStream &is) {
    unsigned char character[1] = {0};
    std::string msg;
    unsigned char a[2] = {0, 0};

    do {
        readBytes(is, character, 1);

        if ( character[0] != 0x00 && ((character[0] >> 7) == 0) ) {
            msg.push_back(static_cast<char>(character[0]));
        } else if ( character[0] != 0x00 ) {
            a[0] = character[0];
            try {
                readBytes(is, character, 1);
                a[1] = character[0];
                const std::string cc = buildUtf8Char(a);
                if ( !cc.empty() ) {
                    msg += cc;
                } else {
                    std::cout << "* UNHANDLED UTF! ********************************************************** ->" << msg << std::endl;
                }
            } catch ( const std::exception & ) {
                break;
            }
        }
    } while ( character[0] != 0x00 );

    return msg;
}

std::string
PersistenceElement::buildUtf8Char(const unsigned char arr[2]) {
    const int a = signedByte2unsignedInteger(arr[0]);
    const int b = signedByte2unsignedInteger(arr[1]);

    if ( ((a >> 5) == 0x06) && ((b >> 6) == 0x02) ) {
        return std::string(reinterpret_cast<const char *>(arr), 2);
    }

    return "";
}

std::string
PersistenceElement::readUtf8Line(java::io::InputStream &is) {
    unsigned char character[1] = {0};
    std::string msg;
    unsigned char a[2] = {0, 0};

    do {
        try {
            readBytes(is, character, 1);
        } catch ( const std::exception & ) {
            return "";
        }

        if ( character[0] != '\n' && character[0] != '\r' && ((character[0] >> 7) == 0) ) {
            msg.push_back(static_cast<char>(character[0]));
        } else if ( character[0] != '\n' && character[0] != '\r' ) {
            a[0] = character[0];
            try {
                readBytes(is, character, 1);
                a[1] = character[0];
                const std::string cc = buildUtf8Char(a);
                if ( !cc.empty() ) {
                    msg += cc;
                }
            } catch ( const std::exception & ) {
                break;
            }
        }
    } while ( character[0] != '\n' );

    return msg;
}

std::string
PersistenceElement::readAsciiLine(java::io::InputStream &is) {
    unsigned char character[1] = {0};
    std::string stringBuffer;

    while ( true ) {
        try {
            readBytes(is, character, 1);
        } catch ( const std::exception & ) {
            break;
        }

        if ( character[0] != '\n' && character[0] != '\r' ) {
            stringBuffer.push_back(static_cast<char>(character[0]));
        }

        if ( character[0] == '\n' ) {
            break;
        }
    }

    return stringBuffer;
}

bool
PersistenceElement::isInSet(unsigned char key, const unsigned char *set, int setLength) {
    for ( int i = 0; i < setLength; i++ ) {
        if ( key == set[i] ) {
            return true;
        }
    }
    return false;
}

std::string
PersistenceElement::readAsciiToken(java::io::InputStream &is, const unsigned char *separators, int separatorsLength) {
    unsigned char character[1] = {0};
    std::string msg;

    do {
        readBytes(is, character, 1);
        if ( !isInSet(character[0], separators, separatorsLength) ) {
            msg.push_back(static_cast<char>(character[0]));
        }
    } while ( !isInSet(character[0], separators, separatorsLength) );

    return msg;
}

void
PersistenceElement::writeAsciiString(FILE *writer, const std::string &cad) {
    writeBytes(writer, reinterpret_cast<const unsigned char *>(cad.data()), static_cast<int>(cad.size()));
    unsigned char end[1] = {'\0'};
    writeBytes(writer, end, 1);
}

void
PersistenceElement::writeUtf8String(FILE *writer, const std::string &cad) {
    writeBytes(writer, reinterpret_cast<const unsigned char *>(cad.data()), static_cast<int>(cad.size()));
    unsigned char end[1] = {'\0'};
    writeBytes(writer, end, 1);
}

void
PersistenceElement::writeAsciiLine(FILE *writer, const std::string &cad) {
    writeBytes(writer, reinterpret_cast<const unsigned char *>(cad.data()), static_cast<int>(cad.size()));
    unsigned char end[1] = {'\n'};
    writeBytes(writer, end, 1);
}

void
PersistenceElement::writeUtf8Line(FILE *writer, const std::string &cad) {
    writeBytes(writer, reinterpret_cast<const unsigned char *>(cad.data()), static_cast<int>(cad.size()));
    unsigned char end[1] = {'\n'};
    writeBytes(writer, end, 1);
}

std::string
PersistenceElement::mapLibraryName(const std::string &libname) {
    if ( libname.empty() ) {
        return libname;
    }
#if defined(_WIN32)
    if ( libname.size() > 4 && libname.substr(libname.size() - 4) == ".dll" ) {
        return libname;
    }
    return libname + ".dll";
#elif defined(__APPLE__)
    if ( libname.find(".dylib") != std::string::npos || libname.find(".so") != std::string::npos ) {
        return libname;
    }
    if ( libname.size() > 3 && libname.substr(0, 3) == "lib" ) {
        return libname + ".dylib";
    }
    return "lib" + libname + ".dylib";
#else
    if ( libname.find(".so") != std::string::npos ) {
        return libname;
    }
    if ( libname.size() > 3 && libname.substr(0, 3) == "lib" ) {
        return libname + ".so";
    }
    return "lib" + libname + ".so";
#endif
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
bool
PersistenceElement::verifyLibrary(const std::string &libname) {
    const std::string nativeLibname = mapLibraryName(libname);

#if defined(_WIN32)
    char pathSeparator = ';';
    const char *envPath = std::getenv("PATH");
#else
    char pathSeparator = ':';
    const char *envPath = std::getenv("LD_LIBRARY_PATH");
#endif

    std::string paths = envPath == nullptr ? "" : envPath;
#if !defined(_WIN32)
    paths += ":/lib:/usr/lib:/usr/local/lib:/usr/X11R6/lib:/usr/X11R6/lib64:/usr/openwin/lib:/usr/dt/lib:/lib64:/usr/lib64:/usr/local/lib64";
#endif

    size_t pos = 0;
    while ( pos <= paths.size() ) {
        const size_t next = paths.find(pathSeparator, pos);
        const std::string token = next == std::string::npos
                                  ? paths.substr(pos)
                                  : paths.substr(pos, next - pos);
        if ( !token.empty() ) {
            struct stat st {};
            if ( stat(token.c_str(), &st) == 0 && S_ISDIR(st.st_mode) ) {
#if defined(_WIN32)
                const std::string fullPath = token + "\\" + nativeLibname;
#else
                const std::string fullPath = token + "/" + nativeLibname;
#endif
                if ( stat(fullPath.c_str(), &st) == 0 ) {
                    return true;
                }
            }
        }
        if ( next == std::string::npos ) {
            break;
        }
        pos = next + 1;
    }

    return false;
}

bool
PersistenceElement::checkDirectory(const std::string &dirName) {
    struct stat st {};
    if ( stat(dirName.c_str(), &st) == 0 ) {
        if ( !S_ISDIR(st.st_mode) ) {
            std::cerr << "Directory " << dirName << " can not be created, because a file with that name already exists (not overwriten)." << std::endl;
            return false;
        }
        return true;
    }

#if defined(_WIN32)
    const int result = _mkdir(dirName.c_str());
#else
    const int result = mkdir(dirName.c_str(), 0755);
#endif
    if ( result != 0 ) {
        std::cerr << "Directory " << dirName << " can not be created, check permisions and available free disk space." << std::endl;
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
std::string
PersistenceElement::extractExtensionFromFile(const java::io::File &fd) {
    java::lang::String javaFilename = fd.getName();
    const char *raw = javaFilename.toCString();
    const std::string filename = raw == nullptr ? "" : raw;

    if ( filename.empty() ) {
        javaFilename.dispose();
        return "";
    }

    std::string ext;
    size_t begin = 0;
    while ( begin <= filename.size() ) {
        size_t end = filename.find('.', begin);
        if ( end == std::string::npos ) {
            ext = filename.substr(begin);
            break;
        }
        ext = filename.substr(begin, end - begin);
        begin = end + 1;
    }

    javaFilename.dispose();
    return ext;
}

} // namespace vsdk
