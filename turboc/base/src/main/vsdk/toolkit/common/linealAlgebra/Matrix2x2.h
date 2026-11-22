#ifndef __MATRIX_2_X_2__
#define __MATRIX_2_X_2__

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector2Df.h"

/**
Single precision affine transform of the plane: a 2x2 block M and a
translation t, so that p' = M p + t:

|x'|   |m00  m01|   | x |   |t0|
|  | = |        | * |   | + |  |
|y'|   |m10  m11|   | y |   |t1|

Immutable; every operation computes in float
*/
class Matrix2x2 {
    float m_[2][2];
    float t_[2];

  public:
    /**
    The identity: M = I, t = 0
    */
    Matrix2x2() {
        m_[0][0] = 1.0f;
        m_[0][1] = 0.0f;
        m_[1][0] = 0.0f;
        m_[1][1] = 1.0f;
        t_[0] = 0.0f;
        t_[1] = 0.0f;
    }
    Matrix2x2(float m00, float m01, float m10, float m11, float t0, float t1) {
        m_[0][0] = m00;
        m_[0][1] = m01;
        m_[1][0] = m10;
        m_[1][1] = m11;
        t_[0] = t0;
        t_[1] = t1;
    }
    Matrix2x2(const Matrix2x2& other) {
        m_[0][0] = other.m_[0][0];
        m_[0][1] = other.m_[0][1];
        m_[1][0] = other.m_[1][0];
        m_[1][1] = other.m_[1][1];
        t_[0] = other.t_[0];
        t_[1] = other.t_[1];
    }
    Matrix2x2& operator=(const Matrix2x2& other) {
        m_[0][0] = other.m_[0][0];
        m_[0][1] = other.m_[0][1];
        m_[1][0] = other.m_[1][0];
        m_[1][1] = other.m_[1][1];
        t_[0] = other.t_[0];
        t_[1] = other.t_[1];
        return *this;
    }

    float get(int row, int col) const { return m_[row][col]; }
    float getTranslation(int row) const { return t_[row]; }

    /**
    M p + t
    */
    Vector2Df transformPoint(const Vector2Df& p) const {
        return Vector2Df(
            m_[0][0] * p.x() + m_[0][1] * p.y() + t_[0],
            m_[1][0] * p.x() + m_[1][1] * p.y() + t_[1]);
    }

    /**
    The composition this(other(p)): M = this.M * other.M and
    t = this.M * other.t + this.t
    */
    Matrix2x2 multiply(const Matrix2x2& other) const {
        return Matrix2x2(
            m_[0][0] * other.m_[0][0] + m_[0][1] * other.m_[1][0],
            m_[0][0] * other.m_[0][1] + m_[0][1] * other.m_[1][1],
            m_[1][0] * other.m_[0][0] + m_[1][1] * other.m_[1][0],
            m_[1][0] * other.m_[0][1] + m_[1][1] * other.m_[1][1],
            m_[0][0] * other.t_[0] + m_[0][1] * other.t_[1] + t_[0],
            m_[1][0] * other.t_[0] + m_[1][1] * other.t_[1] + t_[1]);
    }

    BOOLEAN operator==(const Matrix2x2& other) const {
        return m_[0][0] == other.m_[0][0] && m_[0][1] == other.m_[0][1] &&
            m_[1][0] == other.m_[1][0] && m_[1][1] == other.m_[1][1] &&
            t_[0] == other.t_[0] && t_[1] == other.t_[1];
    }
};

#endif
