#include <cerrno>
#include <cstdlib>
#include <cstring>

#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/io/PersistenceElement.h"
#include "vsdk/toolkit/common/logging/Logger.h"
const bool PersistenceElement::bigEndianArchitecture = false;
unsigned char PersistenceElement::byteBuffer1byte[1] = {0};
unsigned char PersistenceElement::byteBuffer2byte[2] = {0, 0};
unsigned char PersistenceElement::byteBuffer4byte[4] = {0, 0, 0, 0};
unsigned char PersistenceElement::byteBuffer8byte[8] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned char PersistenceElement::bytesForLong[4] = {0, 0, 0, 0};

int
PersistenceElement::signedByte2unsignedInteger(unsigned char value) {
    return static_cast<int>(value);
}

int
PersistenceElement::readByteInt(java::InputStream &is) {
    readBytes(is, byteBuffer1byte, 1);
    return static_cast<int>(static_cast<signed char>(byteBuffer1byte[0]));
}

int
PersistenceElement::readByteUnsignedInt(java::InputStream &is) {
    readBytes(is, byteBuffer1byte, 1);
    return signedByte2unsignedInteger(byteBuffer1byte[0]);
}

void
PersistenceElement::writeByte(java::OutputStream &os, unsigned char value) {
    writeBytes(os, &value, 1);
}

void
PersistenceElement::writeBool(java::OutputStream &os, bool value) {
    writeByte(os, static_cast<unsigned char>(value ? 1 : 0));
}

/**
Given a previously initialized array of bytes, this method fills it
with information readed from the given input stream.  If it is not
enough information to read, this method generates an Exception.
@param is
@param bytesBuffer
*/
void
PersistenceElement::readBytes(java::InputStream &is, unsigned char *bytesBuffer, int length) {
    if ( bytesBuffer == nullptr || length < 0 ) {
        Logger::reportMessage("PersistenceElement", Logger::ERROR, "readBytes", "invalid buffer");
        return;
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
        std::memset(bytesBuffer + offset, 0, static_cast<size_t>(length - offset));
        Logger::reportMessage("PersistenceElement", Logger::ERROR, "readBytes", "could not read requested length");
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
PersistenceElement::writeBytes(java::OutputStream &os, const unsigned char *bytesBuffer, int length) {
    if ( bytesBuffer == nullptr || length < 0 ) {
        Logger::reportMessage("PersistenceElement", Logger::ERROR, "writeBytes", "invalid arguments");
        return;
    }
    if ( length == 0 ) {
        return;
    }
    os.write(bytesBuffer, 0, length);
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

    unsigned int accum = 0;
    i = 0;
    for ( int shiftBy = 0; shiftBy < 32; shiftBy += 8 ) {
        accum |= static_cast<unsigned int>(tmp[i] & 0xff) << shiftBy;
        i++;
    }

    float out = 0.0F;
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

    unsigned long long accum = 0;
    i = 0;
    for ( int shiftBy = 0; shiftBy < 64; shiftBy += 8 ) {
        accum |= static_cast<unsigned long long>(tmp[i] & 0xff) << shiftBy;
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

    unsigned int accum = 0;
    i = 0;
    for ( int shiftBy = 0; shiftBy < 32; shiftBy += 8 ) {
        accum |= static_cast<unsigned int>(tmp[i] & 0xff) << shiftBy;
        i++;
    }

    float out = 0.0F;
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

    unsigned long long accum = 0;
    i = 0;
    for ( int shiftBy = 0; shiftBy < 64; shiftBy += 8 ) {
        accum |= static_cast<unsigned long long>(tmp[i] & 0xff) << shiftBy;
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
    unsigned int bits = 0;
    std::memcpy(&bits, &num, sizeof(unsigned int));
    const long a = static_cast<long>(bits);
    if ( bigEndianArchitecture ) {
        signedInt2byteArrayDirect(arr, start, a);
    }
    signedInt2byteArrayInvert(arr, start, a);
}

void
PersistenceElement::float2byteArrayLE(unsigned char *arr, int start, float num) {
    unsigned int bits = 0;
    std::memcpy(&bits, &num, sizeof(unsigned int));
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
PersistenceElement::readSignedShortLE(java::InputStream &is) {
    readBytes(is, byteBuffer2byte, 2);
    return byteArray2signedShortLE(byteBuffer2byte, 0);
}

int
PersistenceElement::readSignedShortBE(java::InputStream &is) {
    unsigned char arr[2] = {0, 0};
    readBytes(is, arr, 2);
    return byteArray2signedShortBE(arr, 0);
}

void
PersistenceElement::writeSignedShortBE(java::OutputStream &os, int num) {
    signedShort2byteArrayBE(byteBuffer2byte, 0, num);
    writeBytes(os, byteBuffer2byte, 2);
}

void
PersistenceElement::writeSignedShortLE(java::OutputStream &os, int num) {
    signedShort2byteArrayLE(byteBuffer2byte, 0, num);
    writeBytes(os, byteBuffer2byte, 2);
}

/**
Pending to check. Is this really managing 64 bit long integers?
@param is
@return
*/
long
PersistenceElement::readLongLE(java::InputStream &is) {
    readBytes(is, bytesForLong, 4);
    return byteArray2longLE(bytesForLong, 0);
}

void
PersistenceElement::writeInt32LE(java::OutputStream &os, int num) {
    writeLongLE(os, static_cast<long>(num));
}

void
PersistenceElement::writeInt64LE(java::OutputStream &os, long long num) {
    const unsigned long long bits = static_cast<unsigned long long>(num);
    const int low = static_cast<int>(bits & 0xFFFFFFFFULL);
    const int high = static_cast<int>((bits >> 32) & 0xFFFFFFFFULL);
    writeInt32LE(os, low);
    writeInt32LE(os, high);
}

void
PersistenceElement::writeDoubleLE(java::OutputStream &os, double num) {
    unsigned long long bits = 0;
    std::memcpy(&bits, &num, sizeof(double));
    writeInt64LE(os, static_cast<long long>(bits));
}

/**
Pending to check. Is this really managing 64 bit long integers?
@param is
@return
*/
long
PersistenceElement::readLongBE(java::InputStream &is) {
    readBytes(is, bytesForLong, 4);
    return byteArray2longBE(bytesForLong, 0);
}

float
PersistenceElement::readFloatLE(java::InputStream &is) {
    readBytes(is, byteBuffer4byte, 4);
    return byteArray2floatLE(byteBuffer4byte, 0);
}

double
PersistenceElement::readDoubleLE(java::InputStream &is) {
    readBytes(is, byteBuffer8byte, 8);
    return byteArray2doubleLE(byteBuffer8byte, 0);
}

double
PersistenceElement::readDoubleBE(java::InputStream &is) {
    readBytes(is, byteBuffer8byte, 8);
    return byteArray2doubleBE(byteBuffer8byte, 0);
}

float
PersistenceElement::readFloatBE(java::InputStream &is) {
    readBytes(is, byteBuffer4byte, 4);
    const long i = byteArray2longBE(byteBuffer4byte, 0);
    const unsigned int j = static_cast<unsigned int>(i);
    float out = 0.0F;
    std::memcpy(&out, &j, sizeof(float));
    return out;
}

void
PersistenceElement::writeFloatBE(java::OutputStream &os, float num) {
    float2byteArrayBE(byteBuffer4byte, 0, num);
    writeBytes(os, byteBuffer4byte, 4);
}

void
PersistenceElement::writeFloatLE(java::OutputStream &os, float num) {
    float2byteArrayLE(byteBuffer4byte, 0, num);
    writeBytes(os, byteBuffer4byte, 4);
}

void
PersistenceElement::writeLongBE(java::OutputStream &os, long num) {
    if ( bigEndianArchitecture ) {
        signedInt2byteArrayDirect(bytesForLong, 0, num);
    }
    signedInt2byteArrayInvert(bytesForLong, 0, num);
    writeBytes(os, bytesForLong, 4);
}

void
PersistenceElement::writeLongLE(java::OutputStream &os, long num) {
    if ( bigEndianArchitecture ) {
        signedInt2byteArrayInvert(bytesForLong, 0, num);
    }
    signedInt2byteArrayDirect(bytesForLong, 0, num);
    writeBytes(os, bytesForLong, 4);
}

char *
PersistenceElement::readAsciiFixedSizeString(java::InputStream &is, int size) {
    if ( size <= 0 ) {
        return duplicateCString("");
    }

    java::ArrayList<unsigned char> bytesForString(size);
    readBytes(is, bytesForString.data(), size);

    char *msg = static_cast<char *>(std::malloc(static_cast<size_t>(size) + 1));
    if ( msg == nullptr ) {
        Logger::reportMessage("PersistenceElement", Logger::ERROR, "readAsciiFixedSizeString", "allocation failure");
        return duplicateCString("");
    }
    std::memcpy(msg, bytesForString.data(), static_cast<size_t>(size));
    msg[size] = '\0';

    unsigned char skip[1] = {0};
    readBytes(is, skip, 1);

    return msg;
}

char *
PersistenceElement::readAsciiString(java::InputStream &is) {
    unsigned char character[1] = {0};
    java::ArrayList<unsigned char> bytes(64);

    do {
        readBytes(is, character, 1);
        if ( character[0] != 0x00 ) {
            if ( !bytes.add(character[0]) ) {
                Logger::reportMessage("PersistenceElement", Logger::ERROR, "readAsciiString", "allocation failure");
                return duplicateCString("");
            }
        }
    } while ( character[0] != 0x00 );

    const size_t length = static_cast<size_t>(bytes.size());
    char *msg = static_cast<char *>(std::malloc(length + 1));
    if ( msg == nullptr ) {
        Logger::reportMessage("PersistenceElement", Logger::ERROR, "readAsciiString", "allocation failure");
        return duplicateCString("");
    }
    for ( long int i = 0; i < bytes.size(); i++ ) {
        msg[i] = static_cast<char>(bytes.get(i));
    }
    msg[length] = '\0';
    return msg;
}

bool
PersistenceElement::buildUtf8Char(const unsigned char arr[2], unsigned char outBytes[2]) {
    const int a = signedByte2unsignedInteger(arr[0]);
    const int b = signedByte2unsignedInteger(arr[1]);

    if ( ((a >> 5) == 0x06) && ((b >> 6) == 0x02) ) {
        outBytes[0] = arr[0];
        outBytes[1] = arr[1];
        return true;
    }

    return false;
}

char *
PersistenceElement::readUtf8String(java::InputStream &is) {
    unsigned char character[1] = {0};
    unsigned char pair[2] = {0, 0};
    unsigned char utf8Char[2] = {0, 0};
    java::ArrayList<unsigned char> bytes(64);

    do {
        readBytes(is, character, 1);

        if ( character[0] != 0x00 && ((character[0] >> 7) == 0) ) {
            if ( !bytes.add(character[0]) ) {
                Logger::reportMessage("PersistenceElement", Logger::ERROR, "readUtf8String", "allocation failure");
                return duplicateCString("");
            }
        } else if ( character[0] != 0x00 ) {
            pair[0] = character[0];
            readBytes(is, character, 1);
            if ( character[0] == 0x00 ) {
                break;
            }
            pair[1] = character[0];
            if ( buildUtf8Char(pair, utf8Char) ) {
                if ( !bytes.add(utf8Char[0]) || !bytes.add(utf8Char[1]) ) {
                    Logger::reportMessage("PersistenceElement", Logger::ERROR, "readUtf8String", "allocation failure");
                    return duplicateCString("");
                }
            } else {
                java::System::err.print("* UNHANDLED UTF sequence while reading UTF-8 string\n");
            }
        }
    } while ( character[0] != 0x00 );

    const size_t length = static_cast<size_t>(bytes.size());
    char *msg = static_cast<char *>(std::malloc(length + 1));
    if ( msg == nullptr ) {
        Logger::reportMessage("PersistenceElement", Logger::ERROR, "readUtf8String", "allocation failure");
        return duplicateCString("");
    }
    for ( long int i = 0; i < bytes.size(); i++ ) {
        msg[i] = static_cast<char>(bytes.get(i));
    }
    msg[length] = '\0';
    return msg;
}

char *
PersistenceElement::readUtf8Line(java::InputStream &is) {
    unsigned char character[1] = {0};
    unsigned char pair[2] = {0, 0};
    unsigned char utf8Char[2] = {0, 0};
    java::ArrayList<unsigned char> bytes(64);

    do {
        readBytes(is, character, 1);
        if ( character[0] == 0x00 ) {
            break;
        }

        if ( character[0] != '\n' && character[0] != '\r' && ((character[0] >> 7) == 0) ) {
            if ( !bytes.add(character[0]) ) {
                Logger::reportMessage("PersistenceElement", Logger::ERROR, "readUtf8Line", "allocation failure");
                return duplicateCString("");
            }
        } else if ( character[0] != '\n' && character[0] != '\r' ) {
            pair[0] = character[0];
            readBytes(is, character, 1);
            if ( character[0] == 0x00 ) {
                break;
            }
            pair[1] = character[0];
            if ( buildUtf8Char(pair, utf8Char) ) {
                if ( !bytes.add(utf8Char[0]) || !bytes.add(utf8Char[1]) ) {
                    Logger::reportMessage("PersistenceElement", Logger::ERROR, "readUtf8Line", "allocation failure");
                    return duplicateCString("");
                }
            }
        }
    } while ( character[0] != '\n' );

    const size_t length = static_cast<size_t>(bytes.size());
    char *msg = static_cast<char *>(std::malloc(length + 1));
    if ( msg == nullptr ) {
        Logger::reportMessage("PersistenceElement", Logger::ERROR, "readUtf8Line", "allocation failure");
        return duplicateCString("");
    }
    for ( long int i = 0; i < bytes.size(); i++ ) {
        msg[i] = static_cast<char>(bytes.get(i));
    }
    msg[length] = '\0';
    return msg;
}

char *
PersistenceElement::readAsciiLine(java::InputStream &is) {
    unsigned char character[1] = {0};
    java::ArrayList<unsigned char> bytes(64);

    while ( true ) {
        readBytes(is, character, 1);
        if ( character[0] == 0x00 ) {
            break;
        }

        if ( character[0] != '\n' && character[0] != '\r' ) {
            if ( !bytes.add(character[0]) ) {
                Logger::reportMessage("PersistenceElement", Logger::ERROR, "readAsciiLine", "allocation failure");
                return duplicateCString("");
            }
        }

        if ( character[0] == '\n' ) {
            break;
        }
    }

    const size_t length = static_cast<size_t>(bytes.size());
    char *msg = static_cast<char *>(std::malloc(length + 1));
    if ( msg == nullptr ) {
        Logger::reportMessage("PersistenceElement", Logger::ERROR, "readAsciiLine", "allocation failure");
        return duplicateCString("");
    }
    for ( long int i = 0; i < bytes.size(); i++ ) {
        msg[i] = static_cast<char>(bytes.get(i));
    }
    msg[length] = '\0';
    return msg;
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

char *
PersistenceElement::readAsciiToken(java::InputStream &is, const unsigned char *separators, int separatorsLength) {
    unsigned char character[1] = {0};
    java::ArrayList<unsigned char> bytes(64);

    do {
        readBytes(is, character, 1);
        if ( character[0] == 0x00 ) {
            break;
        }
        if ( !isInSet(character[0], separators, separatorsLength) ) {
            if ( !bytes.add(character[0]) ) {
                Logger::reportMessage("PersistenceElement", Logger::ERROR, "readAsciiToken", "allocation failure");
                return duplicateCString("");
            }
        }
    } while ( !isInSet(character[0], separators, separatorsLength) );

    const size_t length = static_cast<size_t>(bytes.size());
    char *msg = static_cast<char *>(std::malloc(length + 1));
    if ( msg == nullptr ) {
        Logger::reportMessage("PersistenceElement", Logger::ERROR, "readAsciiToken", "allocation failure");
        return duplicateCString("");
    }
    for ( long int i = 0; i < bytes.size(); i++ ) {
        msg[i] = static_cast<char>(bytes.get(i));
    }
    msg[length] = '\0';
    return msg;
}

void
PersistenceElement::writeAsciiString(java::OutputStream &writer, const char *cad) {
    const char *text = cad == nullptr ? "" : cad;
    const int textLength = static_cast<int>(std::strlen(text));
    if ( textLength > 0 ) {
        writeBytes(writer, reinterpret_cast<const unsigned char *>(text), textLength);
    }
    unsigned char end[1] = {'\0'};
    writeBytes(writer, end, 1);
}

void
PersistenceElement::writeUtf8String(java::OutputStream &writer, const char *cad) {
    const char *text = cad == nullptr ? "" : cad;
    const int textLength = static_cast<int>(std::strlen(text));
    if ( textLength > 0 ) {
        writeBytes(writer, reinterpret_cast<const unsigned char *>(text), textLength);
    }
    unsigned char end[1] = {'\0'};
    writeBytes(writer, end, 1);
}

void
PersistenceElement::writeAsciiLine(java::OutputStream &writer, const char *cad) {
    const char *text = cad == nullptr ? "" : cad;
    const int textLength = static_cast<int>(std::strlen(text));
    if ( textLength > 0 ) {
        writeBytes(writer, reinterpret_cast<const unsigned char *>(text), textLength);
    }
    unsigned char end[1] = {'\n'};
    writeBytes(writer, end, 1);
}

void
PersistenceElement::writeUtf8Line(java::OutputStream &writer, const char *cad) {
    const char *text = cad == nullptr ? "" : cad;
    const int textLength = static_cast<int>(std::strlen(text));
    if ( textLength > 0 ) {
        writeBytes(writer, reinterpret_cast<const unsigned char *>(text), textLength);
    }
    unsigned char end[1] = {'\n'};
    writeBytes(writer, end, 1);
}

char *
PersistenceElement::duplicateCString(const char *text) {
    const char *source = text == nullptr ? "" : text;
    const size_t length = std::strlen(source);
    char *copy = static_cast<char *>(std::malloc(length + 1));
    if ( copy == nullptr ) {
        return nullptr;
    }
    std::memcpy(copy, source, length + 1);
    return copy;
}

char *
PersistenceElement::joinCString2(const char *left, const char *right) {
    const char *leftText = left == nullptr ? "" : left;
    const char *rightText = right == nullptr ? "" : right;

    const size_t leftLength = std::strlen(leftText);
    const size_t rightLength = std::strlen(rightText);
    char *joined = static_cast<char *>(std::malloc(leftLength + rightLength + 1));
    if ( joined == nullptr ) {
        return nullptr;
    }

    std::memcpy(joined, leftText, leftLength);
    std::memcpy(joined + leftLength, rightText, rightLength);
    joined[leftLength + rightLength] = '\0';
    return joined;
}

char *
PersistenceElement::joinCString3(const char *first, const char *second, const char *third) {
    const char *firstText = first == nullptr ? "" : first;
    const char *secondText = second == nullptr ? "" : second;
    const char *thirdText = third == nullptr ? "" : third;

    const size_t firstLength = std::strlen(firstText);
    const size_t secondLength = std::strlen(secondText);
    const size_t thirdLength = std::strlen(thirdText);
    char *joined = static_cast<char *>(std::malloc(firstLength + secondLength + thirdLength + 1));
    if ( joined == nullptr ) {
        return nullptr;
    }

    std::memcpy(joined, firstText, firstLength);
    std::memcpy(joined + firstLength, secondText, secondLength);
    std::memcpy(joined + firstLength + secondLength, thirdText, thirdLength);
    joined[firstLength + secondLength + thirdLength] = '\0';
    return joined;
}

bool
PersistenceElement::containsCString(const char *text, const char *fragment) {
    if ( text == nullptr || fragment == nullptr ) {
        return false;
    }
    return std::strstr(text, fragment) != nullptr;
}

bool
PersistenceElement::startsWithCString(const char *text, const char *prefix) {
    if ( text == nullptr || prefix == nullptr ) {
        return false;
    }
    const size_t textLength = std::strlen(text);
    const size_t prefixLength = std::strlen(prefix);
    if ( prefixLength > textLength ) {
        return false;
    }
    return std::strncmp(text, prefix, prefixLength) == 0;
}

bool
PersistenceElement::endsWithCString(const char *text, const char *suffix) {
    if ( text == nullptr || suffix == nullptr ) {
        return false;
    }
    const size_t textLength = std::strlen(text);
    const size_t suffixLength = std::strlen(suffix);
    if ( suffixLength > textLength ) {
        return false;
    }
    return std::strncmp(text + textLength - suffixLength, suffix, suffixLength) == 0;
}

bool
PersistenceElement::containsExistingLibrary(const char *pathList, char pathSeparator, const char *nativeLibname) {
    if ( pathList == nullptr || nativeLibname == nullptr || nativeLibname[0] == '\0' ) {
        return false;
    }

    const char *tokenStart = pathList;
    while ( tokenStart != nullptr ) {
        const char *cursor = tokenStart;
        while ( *cursor != '\0' && *cursor != pathSeparator ) {
            cursor++;
        }

        const size_t tokenLength = static_cast<size_t>(cursor - tokenStart);
        if ( tokenLength > 0 ) {
            char *token = static_cast<char *>(std::malloc(tokenLength + 1));
            if ( token != nullptr ) {
                std::memcpy(token, tokenStart, tokenLength);
                token[tokenLength] = '\0';

                char *fullPath = joinCString3(token, "/", nativeLibname);
                if ( fullPath != nullptr ) {
                    java::File candidate(fullPath);
                    if ( candidate.exists() && candidate.canRead() && candidate.isFile() ) {
                        std::free(fullPath);
                        std::free(token);
                        return true;
                    }
                    std::free(fullPath);
                }
                std::free(token);
            }
        }

        if ( *cursor == '\0' ) {
            break;
        }
        tokenStart = cursor + 1;
    }

    return false;
}

bool
PersistenceElement::verifyLibrary(const char *libname) {
    if ( libname == nullptr || libname[0] == '\0' ) {
        return false;
    }

    const char *prefix = "";
    const char *suffix = "";
    char separator = ':';
#if defined(_WIN32)
    suffix = ".dll";
    separator = ';';
#elif defined(__APPLE__)
    prefix = "lib";
    suffix = ".dylib";
#else
    prefix = "lib";
    suffix = ".so";
#endif

    char *nativeLibname = joinCString3(prefix, libname, suffix);
    if ( nativeLibname == nullptr ) {
        return false;
    }

    const char *paths = std::getenv("JAVA_LIBRARY_PATH");
    if ( paths == nullptr || paths[0] == '\0' ) {
        paths = std::getenv("LD_LIBRARY_PATH");
    }

    bool exists = false;
    if ( paths != nullptr && paths[0] != '\0' ) {
        exists = containsExistingLibrary(paths, separator, nativeLibname);
    }

#if !defined(_WIN32)
    if ( !exists ) {
        const char *fallbackUnixPaths[] = {
            "/lib", "/usr/lib", "/usr/local/lib", "/usr/X11R6/lib",
            "/usr/X11R6/lib64", "/usr/openwin/lib", "/usr/dt/lib",
            "/lib64", "/usr/lib64", "/usr/local/lib64"
        };
        for ( size_t i = 0; i < sizeof(fallbackUnixPaths) / sizeof(fallbackUnixPaths[0]); i++ ) {
            char *candidate = joinCString3(fallbackUnixPaths[i], "/", nativeLibname);
            if ( candidate != nullptr ) {
                java::File file(candidate);
                if ( file.exists() && file.canRead() && file.isFile() ) {
                    exists = true;
                    std::free(candidate);
                    break;
                }
                std::free(candidate);
            }
        }
    }
#endif

    std::free(nativeLibname);
    return exists;
}

bool
PersistenceElement::checkDirectory(const char *dirName) {
    if ( dirName == nullptr || dirName[0] == '\0' ) {
        java::System::err.print("Directory name is empty.\n");
        return false;
    }

    java::File directory(dirName);
    if ( directory.exists() && !directory.isDirectory() ) {
        java::System::err.printf(
            "Directory %s can not be created, because a file with that name already exists (not overwriten).\n",
            dirName);
        return false;
    }

    if ( !directory.exists() ) {
        if ( !directory.mkdirs() ) {
            java::System::err.printf(
                "Directory %s can not be created, check permisions and available free disk space.\n",
                dirName);
            return false;
        }
    }

    if ( !directory.canRead() || !directory.canWrite() ) {
        java::System::err.printf(
            "Directory %s is not accessible for read/write operations.\n",
            dirName);
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
char *
PersistenceElement::extractExtensionFromFile(const java::File &fd) {
    java::String javaFilename = fd.getName();
    const char *raw = javaFilename.toCString();
    if ( raw == nullptr || raw[0] == '\0' ) {
        javaFilename.dispose();
        return duplicateCString("");
    }

    const char *extension = raw;
    const char *cursor = raw;
    while ( true ) {
        const char *dot = std::strchr(cursor, '.');
        if ( dot == nullptr ) {
            break;
        }
        extension = dot + 1;
        cursor = dot + 1;
    }

    char *output = duplicateCString(extension);
    javaFilename.dispose();
    return output;
}
