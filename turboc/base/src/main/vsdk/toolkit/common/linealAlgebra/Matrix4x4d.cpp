#include <math.h>
#include <stdio.h>
#include <string.h>

#include "java/lang/String.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/VSDKFatalException.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/Cpp98Support.h"
Matrix4x4d::Matrix4x4d()
{
    for ( int i = 0; i < 4; ++i ) {
        for ( int j = 0; j < 4; ++j ) {
            m_[i][j] = (i == j) ? 1.0 : 0.0;
        }
    }
}

Matrix4x4d::Matrix4x4d(const Matrix4x4d& other)
{
    for ( int i = 0; i < 4; ++i ) for ( int j = 0; j < 4; ++j ) m_[i][j] = other.m_[i][j];
}

Matrix4x4d::Matrix4x4d(const double values[4][4])
{
    for ( int i = 0; i < 4; ++i ) for ( int j = 0; j < 4; ++j ) m_[i][j] = values[i][j];
}

Matrix4x4d Matrix4x4d::identityMatrix() { return Matrix4x4d(); }
Matrix4x4d Matrix4x4d::identity() const { return Matrix4x4d(); }

double Matrix4x4d::get(int row, int col) const { return m_[row][col]; }

Matrix4x4d Matrix4x4d::withVal(int row, int col, double val) const
{
    Matrix4x4d r(*this);
    r.m_[row][col] = val;
    return r;
}

double** Matrix4x4d::toArrayCopy() const
{
    double** out = new double*[4];
    for ( int i = 0; i < 4; ++i ) {
        out[i] = new double[4];
        for ( int j = 0; j < 4; ++j ) out[i][j] = m_[i][j];
    }
    return out;
}

Matrix4x4d Matrix4x4d::withoutTranslation() const
{
    Matrix4x4d r(*this);
    r.m_[0][3] = r.m_[1][3] = r.m_[2][3] = 0.0;
    r.m_[3][0] = r.m_[3][1] = r.m_[3][2] = 0.0;
    r.m_[3][3] = 1.0;
    return r;
}

Vector3Dd Matrix4x4d::extractTranslation() const { return Vector3Dd(get(0,3), get(1,3), get(2,3)); }
Matrix4x4d Matrix4x4d::withTranslation(const Vector3Dd& t) const { return withVal(0,3,t.x()).withVal(1,3,t.y()).withVal(2,3,t.z()); }

Matrix4x4d Matrix4x4d::orthogonalProjection(double l, double r, double d, double u, double n, double f) const
{
    double tx = -((r + l) / (r - l));
    double ty = -((u + d) / (u - d));
    double tz = -((f + n) / (f - n));
    Matrix4x4d m;
    m = m.withVal(0,0,2.0/(r-l)).withVal(0,1,0).withVal(0,2,0).withVal(0,3,tx)
         .withVal(1,0,0).withVal(1,1,2.0/(u-d)).withVal(1,2,0).withVal(1,3,ty)
         .withVal(2,0,0).withVal(2,1,0).withVal(2,2,-2.0/(f-n)).withVal(2,3,tz)
         .withVal(3,0,0).withVal(3,1,0).withVal(3,2,0).withVal(3,3,1);
    return m;
}

Matrix4x4d Matrix4x4d::canonicalPerspectiveProjection() const
{
    Matrix4x4d m;
    return m.withVal(0,0,1).withVal(0,1,0).withVal(0,2,0).withVal(0,3,0)
            .withVal(1,0,0).withVal(1,1,1).withVal(1,2,0).withVal(1,3,0)
            .withVal(2,0,0).withVal(2,1,0).withVal(2,2,0).withVal(2,3,0)
            .withVal(3,0,0).withVal(3,1,0).withVal(3,2,-1).withVal(3,3,1);
}

Matrix4x4d Matrix4x4d::frustumProjection(double l, double r, double d, double u, double n, double f) const
{
    double a = (r + l) / (r - l);
    double b = (u + d) / (u - d);
    double c = -((f + n) / (f - n));
    double dd = -((2 * f * n) / (f - n));
    Matrix4x4d m;
    return m.withVal(0,0,2*n/(r-l)).withVal(0,1,0).withVal(0,2,a).withVal(0,3,0)
            .withVal(1,0,0).withVal(1,1,2*n/(u-d)).withVal(1,2,b).withVal(1,3,0)
            .withVal(2,0,0).withVal(2,1,0).withVal(2,2,c).withVal(2,3,dd)
            .withVal(3,0,0).withVal(3,1,0).withVal(3,2,-1).withVal(3,3,0);
}

Matrix4x4d Matrix4x4d::translation(double x, double y, double z) const
{
    return Matrix4x4d().withVal(0,3,x).withVal(1,3,y).withVal(2,3,z);
}

Matrix4x4d Matrix4x4d::scale(double sx, double sy, double sz) const
{
    return Matrix4x4d().withVal(0,0,sx).withVal(1,1,sy).withVal(2,2,sz);
}

Matrix4x4d Matrix4x4d::scale(const Vector3Dd& s) const { return scale(s.x(), s.y(), s.z()); }
Matrix4x4d Matrix4x4d::translation(const Vector3Dd& t) const { return translation(t.x(), t.y(), t.z()); }

Matrix4x4d Matrix4x4d::eulerAnglesRotation(double yaw, double pitch, double roll) const
{
    Matrix4x4d r1 = Matrix4x4d().axisRotation(roll, 1, 0, 0);
    Matrix4x4d r2 = Matrix4x4d().axisRotation(pitch, 0, -1, 0);
    Matrix4x4d r3 = Matrix4x4d().axisRotation(yaw, 0, 0, 1);
    return r3.multiply(r2.multiply(r1));
}

Matrix4x4d Matrix4x4d::axisRotation(double angle, const Vector3Dd& axis) const { return axisRotation(angle, axis.x(), axis.y(), axis.z()); }

Matrix4x4d Matrix4x4d::axisRotation(double angle, double x, double y, double z) const
{
    double norm = sqrt(x*x + y*y + z*z);
    if ( Cpp98Support::abs(norm) < VSDK::EPSILON ) return Matrix4x4d();
    x /= norm; y /= norm; z /= norm;
    double s = sin(angle);
    double c = cos(angle);
    double t = 1.0 - c;

    Matrix4x4d m;
    m = m.withVal(0,0,t*x*x + c).withVal(0,1,t*x*y - s*z).withVal(0,2,t*x*z + s*y).withVal(0,3,0)
         .withVal(1,0,t*x*y + s*z).withVal(1,1,t*y*y + c).withVal(1,2,t*y*z - s*x).withVal(1,3,0)
         .withVal(2,0,t*x*z - s*y).withVal(2,1,t*y*z + s*x).withVal(2,2,t*z*z + c).withVal(2,3,0)
         .withVal(3,0,0).withVal(3,1,0).withVal(3,2,0).withVal(3,3,1);
    return m;
}

Matrix4x4d Matrix4x4d::inverse() const { return invert(); }

Matrix4x4d Matrix4x4d::invert() const
{
    double d = determinant();
    if ( Cpp98Support::abs(d) < VSDK::EPSILON ) {
        Logger::reportMessage("Matrix4x4d", Logger::ERROR, "invert", "Trying to invert a matrix with zero determinant");
        throw VSDKFatalException("Trying to invert a matrix with zero determinant");
    }
    return cofactors().transpose().multiply(1.0 / d);
}

Matrix4x4d Matrix4x4d::cofactors() const
{
    Matrix4x4d r;
    for ( int row = 0; row < 4; ++row ) {
        for ( int col = 0; col < 4; ++col ) {
            double sub[3][3];
            for ( int i = 0, si = 0; i < 4; ++i ) {
                if ( i == row ) continue;
                for ( int j = 0, sj = 0; j < 4; ++j ) {
                    if ( j == col ) continue;
                    sub[si][sj++] = m_[i][j];
                }
                si++;
            }
            double minor = sub[0][0] * (sub[1][1] * sub[2][2] - sub[1][2] * sub[2][1]) -
                           sub[0][1] * (sub[1][0] * sub[2][2] - sub[1][2] * sub[2][0]) +
                           sub[0][2] * (sub[1][0] * sub[2][1] - sub[1][1] * sub[2][0]);
            double sign = ((row + col) % 2 == 0) ? 1.0 : -1.0;
            r = r.withVal(row, col, sign * minor);
        }
    }
    return r;
}

Matrix4x4d Matrix4x4d::transpose() const
{
    Matrix4x4d r;
    for ( int i = 0; i < 4; ++i ) for ( int j = 0; j < 4; ++j ) r.m_[j][i] = m_[i][j];
    return r;
}

Matrix4x4d Matrix4x4d::multiply(double a) const
{
    Matrix4x4d r(*this);
    for ( int i = 0; i < 4; ++i ) for ( int j = 0; j < 4; ++j ) r.m_[i][j] *= a;
    return r;
}

Vector4Dd Matrix4x4d::multiply(const Vector4Dd& e) const
{
    const double ex = e.x();
    const double ey = e.y();
    const double ez = e.z();
    const double ew = e.w();
    return Vector4Dd(
        m_[0][0] * ex + m_[0][1] * ey + m_[0][2] * ez + m_[0][3] * ew,
        m_[1][0] * ex + m_[1][1] * ey + m_[1][2] * ez + m_[1][3] * ew,
        m_[2][0] * ex + m_[2][1] * ey + m_[2][2] * ez + m_[2][3] * ew,
        m_[3][0] * ex + m_[3][1] * ey + m_[3][2] * ez + m_[3][3] * ew
    );
}

Vector3Dd Matrix4x4d::multiply(const Vector3Dd& e) const
{
    const double ex = e.x();
    const double ey = e.y();
    const double ez = e.z();

    const double rx = m_[0][0] * ex + m_[0][1] * ey + m_[0][2] * ez + m_[0][3];
    const double ry = m_[1][0] * ex + m_[1][1] * ey + m_[1][2] * ez + m_[1][3];
    const double rz = m_[2][0] * ex + m_[2][1] * ey + m_[2][2] * ez + m_[2][3];
    const double rw = m_[3][0] * ex + m_[3][1] * ey + m_[3][2] * ez + m_[3][3];

    if ( Cpp98Support::abs(rw) < VSDK::EPSILON ) {
        return Vector3Dd(rx, ry, rz);
    }
    const double invW = 1.0 / rw;
    return Vector3Dd(rx * invW, ry * invW, rz * invW);
}

Matrix4x4d Matrix4x4d::multiply(const Matrix4x4d& second) const
{
    Matrix4x4d r;
    for ( int row = 0; row < 4; ++row ) {
        for ( int col = 0; col < 4; ++col ) {
            double accum = 0.0;
            for ( int k = 0; k < 4; ++k ) accum += m_[row][k] * second.m_[k][col];
            r.m_[row][col] = accum;
        }
    }
    return r;
}

double Matrix4x4d::determinant() const
{
    double d = 0.0;
    for ( int col = 0; col < 4; ++col ) {
        double sub[3][3];
        for ( int i = 1; i < 4; ++i ) {
            int sj = 0;
            for ( int j = 0; j < 4; ++j ) {
                if ( j == col ) continue;
                sub[i - 1][sj++] = m_[i][j];
            }
        }

        double minor = sub[0][0] * (sub[1][1] * sub[2][2] - sub[1][2] * sub[2][1]) -
                       sub[0][1] * (sub[1][0] * sub[2][2] - sub[1][2] * sub[2][0]) +
                       sub[0][2] * (sub[1][0] * sub[2][1] - sub[1][1] * sub[2][0]);

        d += ((col % 2) == 0 ? 1.0 : -1.0) * m_[0][col] * minor;
    }
    return d;
}

String* Matrix4x4d::toString() const
{
    char s[4096];
    int pos = 0;
    pos += snprintf(s + pos, sizeof(s) - pos, "\n------------------------------\n");
    for ( int i = 0; i < 4; ++i ) {
        for ( int j = 0; j < 4; ++j ) {
            pos += snprintf(s + pos, sizeof(s) - pos, "%s ", VSDK::formatDouble(m_[i][j]).c_str());
        }
        pos += snprintf(s + pos, sizeof(s) - pos, "\n");
    }
    snprintf(s + pos, sizeof(s) - pos, "------------------------------\n");
    return new String(s);
}

double* Matrix4x4d::exportToDoubleArrayRowOrder() const
{
    double* array = new double[16];
    for ( int i = 0, k = 0; i < 4; i++ ) for ( int j = 0; j < 4; j++, k++ ) array[k] = m_[i][j];
    return array;
}

float* Matrix4x4d::exportToFloatArrayRowOrder() const
{
    float* array = new float[16];
    for ( int i = 0, k = 0; i < 4; i++ ) for ( int j = 0; j < 4; j++, k++ ) array[k] = (float)m_[i][j];
    return array;
}

double* Matrix4x4d::exportToDoubleArrayColumnOrder() const
{
    double* array = new double[16];
    for ( int j = 0, k = 0; j < 4; j++ ) for ( int i = 0; i < 4; i++, k++ ) array[k] = m_[i][j];
    return array;
}

float* Matrix4x4d::exportToFloatArrayColumnOrder() const
{
    float* array = new float[16];
    for ( int j = 0, k = 0; j < 4; j++ ) for ( int i = 0; i < 4; i++, k++ ) array[k] = (float)m_[i][j];
    return array;
}

Quaterniond Matrix4x4d::exportToQuaternion() const
{
    double tr = m_[0][0] + m_[1][1] + m_[2][2];
    double qx, qy, qz, qw;
    double q[4] = {0, 0, 0, 0};
    int nxt[3] = {1, 2, 0};

    if ( tr > 0.0 ) {
        double s = sqrt(tr + 1.0);
        qw = s / 2.0;
        s = 0.5 / s;
        qx = (m_[2][1] - m_[1][2]) * s;
        qy = (m_[0][2] - m_[2][0]) * s;
        qz = (m_[1][0] - m_[0][1]) * s;
    }
    else {
        int i = 0;
        if ( m_[1][1] > m_[0][0] ) i = 1;
        if ( m_[2][2] > m_[i][i] ) i = 2;
        int j = nxt[i];
        int k = nxt[j];

        double s = sqrt((m_[i][i] - (m_[j][j] + m_[k][k])) + 1.0);
        q[i] = s * 0.5;
        if ( s != 0.0 ) s = 0.5 / s;

        q[3] = (m_[k][j] - m_[j][k]) * s;
        q[j] = (m_[j][i] + m_[i][j]) * s;
        q[k] = (m_[k][i] + m_[i][k]) * s;

        qx = q[0]; qy = q[1]; qz = q[2]; qw = q[3];
    }

    return Quaterniond(Vector3Dd(qx, qy, qz), qw);
}

Matrix4x4d Matrix4x4d::importFromQuaternion(const Quaterniond& a) const
{
    double x2 = a.direction().x() + a.direction().x();
    double y2 = a.direction().y() + a.direction().y();
    double z2 = a.direction().z() + a.direction().z();
    double xx = a.direction().x() * x2;
    double xy = a.direction().x() * y2;
    double xz = a.direction().x() * z2;
    double yy = a.direction().y() * y2;
    double yz = a.direction().y() * z2;
    double zz = a.direction().z() * z2;
    double sx = a.magnitude() * x2;
    double sy = a.magnitude() * y2;
    double sz = a.magnitude() * z2;

    Matrix4x4d r;
    return r.withVal(0,0,1 - (yy + zz)).withVal(0,1,xy - sz).withVal(0,2,xz + sy).withVal(0,3,0)
            .withVal(1,0,xy + sz).withVal(1,1,1 - (xx + zz)).withVal(1,2,yz - sx).withVal(1,3,0)
            .withVal(2,0,xz - sy).withVal(2,1,yz + sx).withVal(2,2,1 - (xx + yy)).withVal(2,3,0)
            .withVal(3,0,0).withVal(3,1,0).withVal(3,2,0).withVal(3,3,1);
}

double Matrix4x4d::obtainEulerYawAngle() const
{
    Vector3Dd dir = Vector3Dd(1, 0, 0);
    double pitch = obtainEulerPitchAngle();
    double epsilon = 0.0004;

    dir = multiply(dir).withZ(0);

    if ( Cpp98Support::abs(M_PI / 2.0 - pitch) < epsilon ) dir = multiply(Vector3Dd(0, 0, -1));
    if ( Cpp98Support::abs(-M_PI / 2.0 - pitch) < epsilon ) dir = multiply(Vector3Dd(0, 0, 1));

    dir = dir.normalized();
    if ( dir.y() <= 0 ) return asin(dir.x()) - M_PI / 2.0;
    return M_PI / 2.0 - asin(dir.x());
}

double Matrix4x4d::obtainEulerPitchAngle() const
{
    Vector3Dd dir = multiply(Vector3Dd(1, 0, 0)).normalized();
    return M_PI / 2.0 - acos(dir.z());
}

double Matrix4x4d::obtainEulerRollAngle() const
{
    double pitch = obtainEulerPitchAngle();
    double yaw = obtainEulerYawAngle();

    Matrix4x4d r3 = Matrix4x4d().axisRotation(yaw, 0, 0, 1).invert();
    Matrix4x4d r2 = Matrix4x4d().axisRotation(pitch, 0, -1, 0).invert();
    Matrix4x4d r1 = r2.multiply(r3.multiply(*this));

    Quaterniond q = r1.exportToQuaternion().normalized();
    r1 = r1.importFromQuaternion(q);

    if ( r1.get(2, 1) >= 0 ) return acos(r1.get(1, 1));
    return -acos(r1.get(1, 1));
}

BOOLEAN Matrix4x4d::epsilonEquals(const Matrix4x4d& other, double epsilon) const
{
    if ( epsilon < 0.0 ) {
        Logger::reportMessage("Matrix4x4d", Logger::ERROR, "epsilonEquals", "epsilon must be >= 0");
        throw VSDKFatalException("epsilon must be >= 0");
    }
    for ( int i = 0; i < 4; ++i ) {
        for ( int j = 0; j < 4; ++j ) {
            if ( Cpp98Support::abs(m_[i][j] - other.m_[i][j]) > epsilon ) return FALSE;
        }
    }
    return TRUE;
}

BOOLEAN Matrix4x4d::operator==(const Matrix4x4d& other) const
{
    for ( int i = 0; i < 4; ++i ) for ( int j = 0; j < 4; ++j ) if ( m_[i][j] != other.m_[i][j] ) return FALSE;
    return TRUE;
}

static unsigned int hashDouble(double val) {
    unsigned char bytes[sizeof(double)];
    memcpy(bytes, &val, sizeof(double));
    unsigned int h = 0u;
    for (int i = 0; i < (int)sizeof(double); ++i)
        h = h * 31u + ((unsigned int)(bytes[i]));
    return h;
}

int Matrix4x4d::hashCode() const
{
    unsigned int result = 1u;
    for ( int i = 0; i < 4; i++ ) {
        for ( int j = 0; j < 4; j++ ) {
            result = 31u * result + hashDouble(m_[i][j]);
        }
    }
    return (int)result;
}
