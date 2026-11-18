#ifndef __MATRIX_4_X_4_D__
#define __MATRIX_4_X_4_D__


#include <cmath>

#include "java/lang/String.h"
#include "vsdk/toolkit/common/linealAlgebra/Quaterniond.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector4Dd.h"
class Matrix4x4d {
    double m_[4][4];

public:
    Matrix4x4d();
    Matrix4x4d(const Matrix4x4d& other);
    Matrix4x4d& operator=(const Matrix4x4d& other);
    explicit Matrix4x4d(const double values[4][4]);

    static Matrix4x4d copyOf(const Matrix4x4d& other) { return Matrix4x4d(other); }
    static Matrix4x4d copyOf(const double values[4][4]) { return Matrix4x4d(values); }
    static Matrix4x4d identityMatrix();

    Matrix4x4d identity() const;
    double get(int row, int col) const;
    Matrix4x4d withVal(int row, int col, double val) const;
    double** toArrayCopy() const;
    Matrix4x4d withoutTranslation() const;
    Vector3Dd extractTranslation() const;
    Matrix4x4d withTranslation(const Vector3Dd& t) const;

    Matrix4x4d orthogonalProjection(double l, double r, double d, double u, double n, double f) const;
    Matrix4x4d canonicalPerspectiveProjection() const;
    Matrix4x4d frustumProjection(double l, double r, double d, double u, double n, double f) const;

    Matrix4x4d translation(double x, double y, double z) const;
    Matrix4x4d scale(double sx, double sy, double sz) const;
    Matrix4x4d scale(const Vector3Dd& s) const;
    Matrix4x4d translation(const Vector3Dd& t) const;

    Matrix4x4d eulerAnglesRotation(double yaw, double pitch, double roll) const;
    Matrix4x4d axisRotation(double angle, const Vector3Dd& axis) const;
    Matrix4x4d axisRotation(double angle, double x, double y, double z) const;
    void axisRotationRodrigues(Matrix4x4d *matrixInverse, Vector3Dd *vector);

    Matrix4x4d inverse() const;
    Matrix4x4d invert() const;
    Matrix4x4d cofactors() const;
    Matrix4x4d transpose() const;

    Matrix4x4d multiply(double a) const;
    Vector3Dd multiply(const Vector3Dd& e) const;
    Vector4Dd multiply(const Vector4Dd& e) const;
    Matrix4x4d multiply(const Matrix4x4d& second) const;

    double determinant() const;
    java::String* toString() const;
    double* exportToDoubleArrayRowOrder() const;
    float* exportToFloatArrayRowOrder() const;
    double* exportToDoubleArrayColumnOrder() const;
    float* exportToFloatArrayColumnOrder() const;

    Quaterniond exportToQuaternion() const;
    Matrix4x4d importFromQuaternion(const Quaterniond& q) const;

    double obtainEulerYawAngle() const;
    double obtainEulerPitchAngle() const;
    double obtainEulerRollAngle() const;

    bool epsilonEquals(const Matrix4x4d& other) const { return epsilonEquals(other, 1e-6); }
    bool epsilonEquals(const Matrix4x4d& other, double epsilon) const;

    bool operator==(const Matrix4x4d& other) const;
    bool equals(const Matrix4x4d& other) const { return (*this) == other; }
    int hashCode() const;

    inline Vector3Dd transformPoint(const Vector3Dd& v) const {
        double vx = v.x(), vy = v.y(), vz = v.z();
        return Vector3Dd(
            m_[0][0]*vx + m_[1][0]*vy + m_[2][0]*vz + m_[3][0],
            m_[0][1]*vx + m_[1][1]*vy + m_[2][1]*vz + m_[3][1],
            m_[0][2]*vx + m_[1][2]*vy + m_[2][2]*vz + m_[3][2]
        );
    }

    inline Vector3Dd transformDirection(const Vector3Dd& v) const {
        double vx = v.x(), vy = v.y(), vz = v.z();
        return Vector3Dd(
            m_[0][0]*vx + m_[1][0]*vy + m_[2][0]*vz,
            m_[0][1]*vx + m_[1][1]*vy + m_[2][1]*vz,
            m_[0][2]*vx + m_[1][2]*vy + m_[2][2]*vz
        );
    }
};


/* Composes a rotation as a sequence of three per-axis rotations (X, then Y,
   then Z), tracking the inverse alongside via transposition (rotation
   matrices are orthogonal, so M^-1 == M^T). This per-axis cos/sin placement
   and composition order follows the classical construction described by
   Olinde Rodrigues in "Des lois géométriques qui régissent les déplacements
   d'un système solide dans l'espace, et de la variation des coordonnées
   provenant de ces déplacements considérés indépendamment des causes qui
   peuvent les produire" (Journal de Mathématiques Pures et Appliquées, 1840).
   `*this` accumulates the composed rotation matrix — the role `matrix` played
   in the legacy code — and `*matrixInverse` accumulates its inverse. */
inline void
Matrix4x4d::axisRotationRodrigues(Matrix4x4d *matrixInverse, Vector3Dd *vector)
{
    Matrix4x4d tempMatrix;
    Vector3Dd radianVector;
    double cosx;
    double cosy;
    double cosz;
    double sinx;
    double siny;
    double sinz;

    radianVector = (*vector).multiply(M_PI / 180.0);
    *this = Matrix4x4d::identityMatrix();
    cosx = std::cos(radianVector.x());
    sinx = std::sin(radianVector.x());
    cosy = std::cos(radianVector.y());
    siny = std::sin(radianVector.y());
    cosz = std::cos(radianVector.z());
    sinz = std::sin(radianVector.z());

    *this = this->withVal(1, 1, cosx);
    *this = this->withVal(2, 2, cosx);
    *this = this->withVal(1, 2, sinx);
    *this = this->withVal(2, 1, 0.0 - sinx);
    *matrixInverse = this->transpose();

    tempMatrix = Matrix4x4d::identityMatrix();
    tempMatrix = tempMatrix.withVal(0, 0, cosy);
    tempMatrix = tempMatrix.withVal(2, 2, cosy);
    tempMatrix = tempMatrix.withVal(0, 2, 0.0 - siny);
    tempMatrix = tempMatrix.withVal(2, 0, siny);
    *this = this->multiply(tempMatrix);
    tempMatrix = tempMatrix.transpose();
    *matrixInverse = tempMatrix.multiply(*matrixInverse);

    tempMatrix = Matrix4x4d::identityMatrix();
    tempMatrix = tempMatrix.withVal(0, 0, cosz);
    tempMatrix = tempMatrix.withVal(1, 1, cosz);
    tempMatrix = tempMatrix.withVal(0, 1, sinz);
    tempMatrix = tempMatrix.withVal(1, 0, 0.0 - sinz);
    *this = this->multiply(tempMatrix);
    tempMatrix = tempMatrix.transpose();
    *matrixInverse = tempMatrix.multiply(*matrixInverse);
}


#endif
