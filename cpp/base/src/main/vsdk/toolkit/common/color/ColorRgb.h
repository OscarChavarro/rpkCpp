#ifndef __COLOR_RGB__
#define __COLOR_RGB__

class ColorRgb {
private:
    double r_;
    double g_;
    double b_;

public:
    ColorRgb() : r_(0), g_(0), b_(0) {}
    ColorRgb(double r, double g, double b) : r_(r), g_(g), b_(b) {}
    ~ColorRgb() {}

    double r() const { return r_; }
    double g() const { return g_; }
    double b() const { return b_; }

    double getR() const { return r_; }
    double getG() const { return g_; }
    double getB() const { return b_; }

    double distance(const ColorRgb& other) const;
    ColorRgb add(const ColorRgb& other) const { return ColorRgb(r_ + other.r_, g_ + other.g_, b_ + other.b_); }
    ColorRgb multiply(double scalar) const { return ColorRgb(r_ * scalar, g_ * scalar, b_ * scalar); }
};

#endif
