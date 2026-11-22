#include <cmath>
#include <cstdio>
#include <cstring>

#include "java/lang/String.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/VSDKFatalException.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4f.h"

Matrix4x4f::Matrix4x4f()
{
    for ( int i = 0; i < 4; ++i ) {
        for ( int j = 0; j < 4; ++j ) {
            m_[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
}

Matrix4x4f::Matrix4x4f(const Matrix4x4f& other)
{
    for ( int i = 0; i < 4; ++i ) for ( int j = 0; j < 4; ++j ) m_[i][j] = other.m_[i][j];
}

Matrix4x4f::Matrix4x4f(const float values[4][4])
{
    for ( int i = 0; i < 4; ++i ) for ( int j = 0; j < 4; ++j ) m_[i][j] = values[i][j];
}

Matrix4x4f& Matrix4x4f::operator=(const Matrix4x4f& other)
{
    for ( int i = 0; i < 4; ++i ) for ( int j = 0; j < 4; ++j ) m_[i][j] = other.m_[i][j];
    return *this;
}

Matrix4x4f Matrix4x4f::withVal(int row, int col, float val) const
{
    Matrix4x4f r(*this);
    r.m_[row][col] = val;
    return r;
}

float** Matrix4x4f::toArrayCopy() const
{
    float** out = new float*[4];
    for ( int i = 0; i < 4; ++i ) {
        out[i] = new float[4];
        for ( int j = 0; j < 4; ++j ) out[i][j] = m_[i][j];
    }
    return out;
}

Matrix4x4f Matrix4x4f::withoutTranslation() const
{
    Matrix4x4f r(*this);
    r.m_[0][3] = r.m_[1][3] = r.m_[2][3] = 0.0f;
    r.m_[3][0] = r.m_[3][1] = r.m_[3][2] = 0.0f;
    r.m_[3][3] = 1.0f;
    return r;
}

Vector3Df Matrix4x4f::extractTranslation() const { return Vector3Df(m_[0][3], m_[1][3], m_[2][3]); }

Matrix4x4f Matrix4x4f::withTranslation(const Vector3Df& t) const
{
    return withVal(0, 3, t.x()).withVal(1, 3, t.y()).withVal(2, 3, t.z());
}

Matrix4x4f Matrix4x4f::orthogonalProjection(float l, float r, float d, float u, float n, float f) const
{
    const float values[4][4] = {
        {2.0f / (r - l), 0.0f, 0.0f, -((r + l) / (r - l))},
        {0.0f, 2.0f / (u - d), 0.0f, -((u + d) / (u - d))},
        {0.0f, 0.0f, -2.0f / (f - n), -((f + n) / (f - n))},
        {0.0f, 0.0f, 0.0f, 1.0f}
    };
    return Matrix4x4f(values);
}

Matrix4x4f Matrix4x4f::canonicalPerspectiveProjection() const
{
    const float values[4][4] = {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 1.0f}
    };
    return Matrix4x4f(values);
}

Matrix4x4f Matrix4x4f::frustumProjection(float l, float r, float d, float u, float n, float f) const
{
    const float values[4][4] = {
        {2 * n / (r - l), 0.0f, (r + l) / (r - l), 0.0f},
        {0.0f, 2 * n / (u - d), (u + d) / (u - d), 0.0f},
        {0.0f, 0.0f, -((f + n) / (f - n)), -((2 * f * n) / (f - n))},
        {0.0f, 0.0f, -1.0f, 0.0f}
    };
    return Matrix4x4f(values);
}

Matrix4x4f Matrix4x4f::translation(float x, float y, float z) const
{
    return Matrix4x4f().withVal(0, 3, x).withVal(1, 3, y).withVal(2, 3, z);
}

Matrix4x4f Matrix4x4f::scale(float sx, float sy, float sz) const
{
    return Matrix4x4f().withVal(0, 0, sx).withVal(1, 1, sy).withVal(2, 2, sz);
}

Matrix4x4f Matrix4x4f::eulerAnglesRotation(float yaw, float pitch, float roll) const
{
    Matrix4x4f r1 = Matrix4x4f().axisRotation(roll, 1, 0, 0);
    Matrix4x4f r2 = Matrix4x4f().axisRotation(pitch, 0, -1, 0);
    Matrix4x4f r3 = Matrix4x4f().axisRotation(yaw, 0, 0, 1);
    return r3.multiply(r2.multiply(r1));
}

Matrix4x4f Matrix4x4f::axisRotation(float angle, const Vector3Df& axis) const
{
    return axisRotation(angle, axis.x(), axis.y(), axis.z());
}

Matrix4x4f Matrix4x4f::axisRotation(float angle, float x, float y, float z) const
{
    const float norm = std::sqrt(x * x + y * y + z * z);
    if ( std::abs(norm) < (float)VSDK::EPSILON ) return Matrix4x4f();
    x /= norm;
    y /= norm;
    z /= norm;
    const float s = std::sin(angle);
    const float c = std::cos(angle);
    const float t = 1.0f - c;
    const float values[4][4] = {
        {t * x * x + c, t * x * y - s * z, t * x * z + s * y, 0.0f},
        {t * x * y + s * z, t * y * y + c, t * y * z - s * x, 0.0f},
        {t * x * z - s * y, t * y * z + s * x, t * z * z + c, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    };
    return Matrix4x4f(values);
}

Matrix4x4f Matrix4x4f::inverse() const { return invert(); }

Matrix4x4f Matrix4x4f::invert() const
{
    const float d = determinant();
    if ( std::abs(d) < (float)VSDK::EPSILON ) {
        Logger::reportMessage("Matrix4x4f", Logger::ERROR, "invert", "Trying to invert a matrix with zero determinant");
        throw VSDKFatalException("Trying to invert a matrix with zero determinant");
    }
    return cofactors().transpose().multiply(1.0f / d);
}

static float minor3x3(const float sub[3][3])
{
    return sub[0][0] * (sub[1][1] * sub[2][2] - sub[1][2] * sub[2][1]) -
           sub[0][1] * (sub[1][0] * sub[2][2] - sub[1][2] * sub[2][0]) +
           sub[0][2] * (sub[1][0] * sub[2][1] - sub[1][1] * sub[2][0]);
}

Matrix4x4f Matrix4x4f::cofactors() const
{
    Matrix4x4f r;
    for ( int row = 0; row < 4; ++row ) {
        for ( int col = 0; col < 4; ++col ) {
            float sub[3][3];
            for ( int i = 0, si = 0; i < 4; ++i ) {
                if ( i == row ) continue;
                for ( int j = 0, sj = 0; j < 4; ++j ) {
                    if ( j == col ) continue;
                    sub[si][sj++] = m_[i][j];
                }
                si++;
            }
            const float sign = ((row + col) % 2 == 0) ? 1.0f : -1.0f;
            r.m_[row][col] = sign * minor3x3(sub);
        }
    }
    return r;
}

Matrix4x4f Matrix4x4f::transpose() const
{
    Matrix4x4f r;
    for ( int i = 0; i < 4; ++i ) for ( int j = 0; j < 4; ++j ) r.m_[j][i] = m_[i][j];
    return r;
}

Matrix4x4f Matrix4x4f::multiply(float a) const
{
    Matrix4x4f r(*this);
    for ( int i = 0; i < 4; ++i ) for ( int j = 0; j < 4; ++j ) r.m_[i][j] *= a;
    return r;
}

Vector3Df Matrix4x4f::multiply(const Vector3Df& e) const
{
    const float ex = e.x();
    const float ey = e.y();
    const float ez = e.z();

    const float rx = m_[0][0] * ex + m_[0][1] * ey + m_[0][2] * ez + m_[0][3];
    const float ry = m_[1][0] * ex + m_[1][1] * ey + m_[1][2] * ez + m_[1][3];
    const float rz = m_[2][0] * ex + m_[2][1] * ey + m_[2][2] * ez + m_[2][3];
    const float rw = m_[3][0] * ex + m_[3][1] * ey + m_[3][2] * ez + m_[3][3];

    if ( std::abs(rw) < (float)VSDK::EPSILON ) {
        return Vector3Df(rx, ry, rz);
    }
    const float invW = 1.0f / rw;
    return Vector3Df(rx * invW, ry * invW, rz * invW);
}

Vector4Df Matrix4x4f::multiply(const Vector4Df& e) const
{
    const float ex = e.x();
    const float ey = e.y();
    const float ez = e.z();
    const float ew = e.w();
    return Vector4Df(
        m_[0][0] * ex + m_[0][1] * ey + m_[0][2] * ez + m_[0][3] * ew,
        m_[1][0] * ex + m_[1][1] * ey + m_[1][2] * ez + m_[1][3] * ew,
        m_[2][0] * ex + m_[2][1] * ey + m_[2][2] * ez + m_[2][3] * ew,
        m_[3][0] * ex + m_[3][1] * ey + m_[3][2] * ez + m_[3][3] * ew
    );
}

Matrix4x4f Matrix4x4f::multiply(const Matrix4x4f& second) const
{
    Matrix4x4f r;
    for ( int row = 0; row < 4; ++row ) {
        for ( int col = 0; col < 4; ++col ) {
            r.m_[row][col] =
                m_[row][0] * second.m_[0][col] +
                m_[row][1] * second.m_[1][col] +
                m_[row][2] * second.m_[2][col] +
                m_[row][3] * second.m_[3][col];
        }
    }
    return r;
}

float Matrix4x4f::determinant() const
{
    float d = 0.0f;
    for ( int col = 0; col < 4; ++col ) {
        float sub[3][3];
        for ( int i = 1; i < 4; ++i ) {
            int sj = 0;
            for ( int j = 0; j < 4; ++j ) {
                if ( j == col ) continue;
                sub[i - 1][sj++] = m_[i][j];
            }
        }
        d += ((col % 2) == 0 ? 1.0f : -1.0f) * m_[0][col] * minor3x3(sub);
    }
    return d;
}

java::String* Matrix4x4f::toString() const
{
    char s[4096];
    int pos = 0;
    pos += std::snprintf(s + pos, sizeof(s) - pos, "\n------------------------------\n");
    for ( int i = 0; i < 4; ++i ) {
        for ( int j = 0; j < 4; ++j ) {
            pos += std::snprintf(s + pos, sizeof(s) - pos, "%s ", VSDK::formatDouble(m_[i][j]).c_str());
        }
        pos += std::snprintf(s + pos, sizeof(s) - pos, "\n");
    }
    std::snprintf(s + pos, sizeof(s) - pos, "------------------------------\n");
    return new java::String(s);
}

double* Matrix4x4f::exportToDoubleArrayRowOrder() const
{
    double* array = new double[16];
    for ( int i = 0, k = 0; i < 4; i++ ) for ( int j = 0; j < 4; j++, k++ ) array[k] = m_[i][j];
    return array;
}

float* Matrix4x4f::exportToFloatArrayRowOrder() const
{
    float* array = new float[16];
    for ( int i = 0, k = 0; i < 4; i++ ) for ( int j = 0; j < 4; j++, k++ ) array[k] = m_[i][j];
    return array;
}

double* Matrix4x4f::exportToDoubleArrayColumnOrder() const
{
    double* array = new double[16];
    for ( int j = 0, k = 0; j < 4; j++ ) for ( int i = 0; i < 4; i++, k++ ) array[k] = m_[i][j];
    return array;
}

float* Matrix4x4f::exportToFloatArrayColumnOrder() const
{
    float* array = new float[16];
    for ( int j = 0, k = 0; j < 4; j++ ) for ( int i = 0; i < 4; i++, k++ ) array[k] = m_[i][j];
    return array;
}

Quaterniond Matrix4x4f::exportToQuaternion() const
{
    const float tr = m_[0][0] + m_[1][1] + m_[2][2];
    float qx;
    float qy;
    float qz;
    float qw;
    float q[4] = {0, 0, 0, 0};
    const int nxt[3] = {1, 2, 0};

    if ( tr > 0.0f ) {
        float s = std::sqrt(tr + 1.0f);
        qw = s / 2.0f;
        s = 0.5f / s;
        qx = (m_[2][1] - m_[1][2]) * s;
        qy = (m_[0][2] - m_[2][0]) * s;
        qz = (m_[1][0] - m_[0][1]) * s;
    } else {
        int i = 0;
        if ( m_[1][1] > m_[0][0] ) i = 1;
        if ( m_[2][2] > m_[i][i] ) i = 2;
        const int j = nxt[i];
        const int k = nxt[j];

        float s = std::sqrt((m_[i][i] - (m_[j][j] + m_[k][k])) + 1.0f);
        q[i] = s * 0.5f;
        if ( s != 0.0f ) s = 0.5f / s;

        q[3] = (m_[k][j] - m_[j][k]) * s;
        q[j] = (m_[j][i] + m_[i][j]) * s;
        q[k] = (m_[k][i] + m_[i][k]) * s;

        qx = q[0];
        qy = q[1];
        qz = q[2];
        qw = q[3];
    }

    return Quaterniond(Vector3Dd(qx, qy, qz), qw);
}

Matrix4x4f Matrix4x4f::importFromQuaternion(const Quaterniond& a) const
{
    const float dx = (float)a.direction().x();
    const float dy = (float)a.direction().y();
    const float dz = (float)a.direction().z();
    const float w = (float)a.magnitude();
    const float x2 = dx + dx;
    const float y2 = dy + dy;
    const float z2 = dz + dz;
    const float xx = dx * x2;
    const float xy = dx * y2;
    const float xz = dx * z2;
    const float yy = dy * y2;
    const float yz = dy * z2;
    const float zz = dz * z2;
    const float sx = w * x2;
    const float sy = w * y2;
    const float sz = w * z2;

    const float values[4][4] = {
        {1 - (yy + zz), xy - sz, xz + sy, 0.0f},
        {xy + sz, 1 - (xx + zz), yz - sx, 0.0f},
        {xz - sy, yz + sx, 1 - (xx + yy), 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    };
    return Matrix4x4f(values);
}

float Matrix4x4f::obtainEulerYawAngle() const
{
    const float pitch = obtainEulerPitchAngle();
    const float epsilon = 0.0004f;
    const float halfPi = (float)(M_PI / 2.0);

    Vector3Df dir = multiply(Vector3Df(1, 0, 0)).withZ(0);

    if ( std::abs(halfPi - pitch) < epsilon ) dir = multiply(Vector3Df(0, 0, -1));
    if ( std::abs(-halfPi - pitch) < epsilon ) dir = multiply(Vector3Df(0, 0, 1));

    dir = dir.normalized();
    if ( dir.y() <= 0 ) return std::asin(dir.x()) - halfPi;
    return halfPi - std::asin(dir.x());
}

float Matrix4x4f::obtainEulerPitchAngle() const
{
    const Vector3Df dir = multiply(Vector3Df(1, 0, 0)).normalized();
    return (float)(M_PI / 2.0) - std::acos(dir.z());
}

float Matrix4x4f::obtainEulerRollAngle() const
{
    const float pitch = obtainEulerPitchAngle();
    const float yaw = obtainEulerYawAngle();

    const Matrix4x4f r3 = Matrix4x4f().axisRotation(yaw, 0, 0, 1).invert();
    const Matrix4x4f r2 = Matrix4x4f().axisRotation(pitch, 0, -1, 0).invert();
    Matrix4x4f r1 = r2.multiply(r3.multiply(*this));

    const Quaterniond q = r1.exportToQuaternion().normalized();
    r1 = r1.importFromQuaternion(q);

    if ( r1.get(2, 1) >= 0 ) return std::acos(r1.get(1, 1));
    return -std::acos(r1.get(1, 1));
}

bool Matrix4x4f::epsilonEquals(const Matrix4x4f& other, float eps) const
{
    if ( eps < 0.0f ) {
        Logger::reportMessage("Matrix4x4f", Logger::ERROR, "epsilonEquals", "epsilon must be >= 0");
        throw VSDKFatalException("epsilon must be >= 0");
    }
    for ( int i = 0; i < 4; ++i ) {
        for ( int j = 0; j < 4; ++j ) {
            if ( std::abs(m_[i][j] - other.m_[i][j]) > eps ) return false;
        }
    }
    return true;
}

bool Matrix4x4f::operator==(const Matrix4x4f& other) const
{
    for ( int i = 0; i < 4; ++i ) for ( int j = 0; j < 4; ++j ) if ( m_[i][j] != other.m_[i][j] ) return false;
    return true;
}

static unsigned int hashFloat(float val)
{
    unsigned char bytes[sizeof(float)];
    std::memcpy(bytes, &val, sizeof(float));
    unsigned int h = 0u;
    for ( int i = 0; i < (int)sizeof(float); ++i ) {
        h = h * 31u + static_cast<unsigned int>(bytes[i]);
    }
    return h;
}

int Matrix4x4f::hashCode() const
{
    unsigned int result = 1u;
    for ( int i = 0; i < 4; i++ ) {
        for ( int j = 0; j < 4; j++ ) {
            result = 31u * result + hashFloat(m_[i][j]);
        }
    }
    return (int)result;
}
