#include "java/lang/IllegalArgumentException.h"
#include <cmath>
#include <cstdio>

#include "java/lang/String.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/linealAlgebra/Quaterniond.h"
#include "vsdk/toolkit/common/linealAlgebra/Quaternionf.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector2Dd.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector2Df.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector4Dd.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector4Df.h"
Vector2Df::Vector2Df(const Vector2Dd& other) : x_((float)other.x), y_((float)other.y) {}

bool Vector3Dd::epsilonEquals(const Vector3Dd& other, double epsilon) const
{
    if ( epsilon < 0.0 ) throw java::IllegalArgumentException("epsilon must be >= 0");
    return std::abs(x_ - other.x_) <= epsilon && std::abs(y_ - other.y_) <= epsilon && std::abs(z_ - other.z_) <= epsilon;
}

double Vector3Dd::obtainSphericalThetaAngle() const
{
    double val;
    if ( std::abs(x_) > VSDK::EPSILON ) {
        val = (x_ > 0.0) ? std::atan(y_ / x_) : M_PI + std::atan(y_ / x_);
    }
    else if ( y_ > 0.0 ) {
        val = M_PI / 2.0;
    }
    else {
        val = M_PI + M_PI / 2.0;
    }
    while ( val < 0.0 ) val += 2.0 * M_PI;
    while ( val > 2.0 * M_PI ) val -= 2.0 * M_PI;
    return val;
}

double Vector3Dd::obtainSphericalPhiAngle() const
{
    double r = length();
    if ( r < VSDK::EPSILON ) return 0.0;
    return std::acos(z_ / r);
}

Vector3Dd Vector3Dd::fromSpherical(double r, double theta, double phi)
{
    return Vector3Dd(r * std::sin(phi) * std::cos(theta), r * std::sin(phi) * std::sin(theta), r * std::cos(phi));
}

Vector3Df::Vector3Df(const Vector3Dd& other) : x_((float)other.x()), y_((float)other.y()), z_((float)other.z()) {}
Vector3Df Vector3Df::multiply(float a) const { return Vector3Df(a * x_, a * y_, a * z_); }
Vector3Df Vector3Df::crossProduct(const Vector3Df& other) const { return Vector3Df(y_ * other.z_ - z_ * other.y_, z_ * other.x_ - x_ * other.z_, x_ * other.y_ - y_ * other.x_); }
float Vector3Df::dotProduct(const Vector3Df& other) const { return x_ * other.x_ + y_ * other.y_ + z_ * other.z_; }
float Vector3Df::length() const { return std::sqrt(x_ * x_ + y_ * y_ + z_ * z_); }
Vector3Df Vector3Df::normalized() const
{
    float t = x_ * x_ + y_ * y_ + z_ * z_;
    if ( std::abs(t) < (float)VSDK::EPSILON ) return *this;
    if ( t != 0.0f && t != 1.0f ) t = 1.0f / std::sqrt(t);
    return Vector3Df(x_ * t, y_ * t, z_ * t);
}
Vector3Df Vector3Df::add(const Vector3Df& b) const { return Vector3Df(x_ + b.x_, y_ + b.y_, z_ + b.z_); }
Vector3Df Vector3Df::subtract(const Vector3Df& b) const { return Vector3Df(x_ - b.x_, y_ - b.y_, z_ - b.z_); }
bool Vector3Df::epsilonEquals(const Vector3Df& other, float epsilon) const
{
    if ( epsilon < 0.0f ) throw java::IllegalArgumentException("epsilon must be >= 0");
    return std::abs(x_ - other.x_) <= epsilon && std::abs(y_ - other.y_) <= epsilon && std::abs(z_ - other.z_) <= epsilon;
}

Vector4Dd::Vector4Dd(const Vector3Dd& other) : x_(other.x()), y_(other.y()), z_(other.z()), w_(1.0) {}
Vector4Dd Vector4Dd::multiply(double a) const { return Vector4Dd(a * x_, a * y_, a * z_, a * w_); }
Vector4Dd Vector4Dd::dividedByW() const { if ( std::abs(w_) < VSDK::EPSILON ) return *this; return Vector4Dd(x_ / w_, y_ / w_, z_ / w_, 1.0); }
double Vector4Dd::length() const { return std::sqrt(x_ * x_ + y_ * y_ + z_ * z_ + w_ * w_); }
Vector4Dd Vector4Dd::add(const Vector4Dd& b) const { return Vector4Dd(x_ + b.x_, y_ + b.y_, z_ + b.z_, w_ + b.w_); }
bool Vector4Dd::epsilonEquals(const Vector4Dd& other, double epsilon) const
{
    if ( epsilon < 0.0 ) throw java::IllegalArgumentException("epsilon must be >= 0");
    return std::abs(x_ - other.x_) <= epsilon && std::abs(y_ - other.y_) <= epsilon && std::abs(z_ - other.z_) <= epsilon && std::abs(w_ - other.w_) <= epsilon;
}

Vector4Df::Vector4Df(const Vector4Dd& other) : x_((float)other.x()), y_((float)other.y()), z_((float)other.z()), w_((float)other.w()) {}
Vector4Df::Vector4Df(const Vector3Df& other) : x_(other.x()), y_(other.y()), z_(other.z()), w_(1.0f) {}
Vector4Df Vector4Df::multiply(float a) const { return Vector4Df(a * x_, a * y_, a * z_, a * w_); }
Vector4Df Vector4Df::dividedByW() const { if ( std::abs(w_) < (float)VSDK::EPSILON ) return *this; return Vector4Df(x_ / w_, y_ / w_, z_ / w_, 1.0f); }
float Vector4Df::length() const { return std::sqrt(x_ * x_ + y_ * y_ + z_ * z_ + w_ * w_); }
Vector4Df Vector4Df::add(const Vector4Df& b) const { return Vector4Df(x_ + b.x_, y_ + b.y_, z_ + b.z_, w_ + b.w_); }
bool Vector4Df::epsilonEquals(const Vector4Df& other, float epsilon) const
{
    if ( epsilon < 0.0f ) throw java::IllegalArgumentException("epsilon must be >= 0");
    return std::abs(x_ - other.x_) <= epsilon && std::abs(y_ - other.y_) <= epsilon && std::abs(z_ - other.z_) <= epsilon && std::abs(w_ - other.w_) <= epsilon;
}

double Quaterniond::lengthSquared() const { return magnitude_ * magnitude_ + direction_.dotProduct(direction_); }
double Quaterniond::length() const { return std::sqrt(lengthSquared()); }
Quaterniond Quaterniond::normalized() const
{
    double l = length();
    if ( std::abs(l) < VSDK::EPSILON ) return *this;
    return Quaterniond(direction_.multiply(1.0 / l), magnitude_ * (1.0 / l));
}
Quaterniond Quaterniond::conjugated() const { return Quaterniond(direction_.multiply(-1.0), magnitude_); }
Vector3Dd Quaterniond::rotate(const Vector3Dd& vector) const
{
    Vector3Dd uv = direction_.crossProduct(vector);
    Vector3Dd uuv = direction_.crossProduct(uv);
    return vector.add(uv.multiply(2.0 * magnitude_)).add(uuv.multiply(2.0));
}
bool Quaterniond::epsilonEquals(const Quaterniond& other, double epsilon) const
{
    if ( epsilon < 0.0 ) throw java::IllegalArgumentException("epsilon must be >= 0");
    return direction_.epsilonEquals(other.direction_, epsilon) && std::abs(magnitude_ - other.magnitude_) <= epsilon;
}

Quaternionf::Quaternionf(const Quaterniond& other) : direction_(Vector3Df(other.direction())), magnitude_((float)other.magnitude()) {}
float Quaternionf::lengthSquared() const { return magnitude_ * magnitude_ + direction_.dotProduct(direction_); }
float Quaternionf::length() const { return std::sqrt(lengthSquared()); }
Quaternionf Quaternionf::normalized() const
{
    float l = length();
    if ( std::abs(l) < (float)VSDK::EPSILON ) return *this;
    float inv = 1.0f / l;
    return Quaternionf(direction_.multiply(inv), magnitude_ * inv);
}
Quaternionf Quaternionf::conjugated() const { return Quaternionf(direction_.multiply(-1.0f), magnitude_); }
Vector3Df Quaternionf::rotate(const Vector3Df& vector) const
{
    Vector3Df uv = direction_.crossProduct(vector);
    Vector3Df uuv = direction_.crossProduct(uv);
    return vector.add(uv.multiply(2.0f * magnitude_)).add(uuv.multiply(2.0f));
}
bool Quaternionf::epsilonEquals(const Quaternionf& other, float epsilon) const
{
    if ( epsilon < 0.0f ) throw java::IllegalArgumentException("epsilon must be >= 0");
    return direction_.epsilonEquals(other.direction_, epsilon) && std::abs(magnitude_ - other.magnitude_) <= epsilon;
}

java::String* Vector2Dd::toString() const
{
    return new java::String(("<" + VSDK::formatDouble(x) + ", " + VSDK::formatDouble(y) + ">").c_str());
}

float* Vector3Dd::exportToFloatArrayVector() const
{
    float* out = new float[4];
    out[0] = (float)x_;
    out[1] = (float)y_;
    out[2] = (float)z_;
    out[3] = 1.0f;
    return out;
}

java::String* Vector3Dd::toString() const
{
    return new java::String(("<" + VSDK::formatDouble(x_) + ", " + VSDK::formatDouble(y_) + ", " + VSDK::formatDouble(z_) + ">").c_str());
}

java::String* Vector4Dd::toString() const
{
    return new java::String(("<" + VSDK::formatDouble(x_) + ", " + VSDK::formatDouble(y_) + ", " + VSDK::formatDouble(z_) + ", " + VSDK::formatDouble(w_) + ">").c_str());
}

java::String* Quaterniond::toString() const
{
    java::String* d = direction_.toString();
    char buf[256];
    std::snprintf(buf, sizeof(buf), "%s / %s", d->toCString(), VSDK::formatDouble(magnitude_).c_str());
    java::String* out = new java::String(buf);
    delete d;
    return out;
}

java::String* Quaternionf::toString() const
{
    char buf[256];
    std::snprintf(
        buf,
        sizeof(buf),
        "<%s, %s, %s> / %s",
        VSDK::formatDouble(direction_.x()).c_str(),
        VSDK::formatDouble(direction_.y()).c_str(),
        VSDK::formatDouble(direction_.z()).c_str(),
        VSDK::formatDouble(magnitude_).c_str());
    return new java::String(buf);
}
