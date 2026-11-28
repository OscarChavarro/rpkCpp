#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/media/RGBColorPalette.h"
RGBColorPalette::RGBColorPalette() {
    init(256);
}

RGBColorPalette::~RGBColorPalette() {
    for (long int i = 0; i < colors.size(); i++) {
        if (colors[i] != nullptr) {
            delete colors[i];
            colors[i] = nullptr;
        }
    }
    colors.clear();
}

void RGBColorPalette::init(int size) {
    for (long int i = 0; i < colors.size(); i++) {
        if (colors[i] != nullptr) {
            delete colors[i];
            colors[i] = nullptr;
        }
    }
    colors.clear();

    for (int i = 0; i < size; i++) {
        colors.add(new ColorRgb(0, 0, 0));
    }
    buildGrayLevelsTable();
}

int RGBColorPalette::size() const {
    return (int)colors.size();
}

void RGBColorPalette::buildGrayLevelsTable() {
    double val = 0.0;
    for (long int i = 0; i < colors.size(); i++) {
        if (colors[i] != nullptr) {
            delete colors[i];
        }
        colors[i] = new ColorRgb(val, val, val);
        val += 1.0 / ((double)(colors.size() - 1));
    }
}

ColorRgb* RGBColorPalette::getColorAt(int i) const {
    if (i < 0 || i >= (int)colors.size()) {
        return nullptr;
    }
    if (colors.get(i) != nullptr) {
        return new ColorRgb(*colors.get(i));
    }
    return nullptr;
}

void RGBColorPalette::setColorAt(int i, ColorRgb* c) {
    if (i < 0 || i >= (int)colors.size()) {
        return;
    }
    if (colors[i] != nullptr) {
        delete colors[i];
    }
    if (c != nullptr) {
        colors[i] = new ColorRgb(*c);
    } else {
        colors[i] = nullptr;
    }
}

void RGBColorPalette::setColorAt(int i, double r, double g, double b) {
    if (i < 0 || i >= (int)colors.size()) {
        return;
    }
    if (colors[i] != nullptr) {
        delete colors[i];
    }
    colors[i] = new ColorRgb(r, g, b);
}

void RGBColorPalette::addColor(ColorRgb* c) {
    if (c != nullptr) {
        colors.add(new ColorRgb(*c));
    } else {
        colors.add(new ColorRgb(0, 0, 0));
    }
}

void RGBColorPalette::addColor(double r, double g, double b) {
    colors.add(new ColorRgb(r, g, b));
}

ColorRgb* RGBColorPalette::evalNearest(double t) const {
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;

    int N = (int)colors.size();
    int i = (int)(t * ((double)N));

    if (i < 0) i = 0;
    if (i >= N) i = N - 1;

    if (colors.get(i) != nullptr) {
        return new ColorRgb(*colors.get(i));
    }
    return new ColorRgb(0, 0, 0);
}

ColorRgb* RGBColorPalette::evalLinear(double t) const {
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;

    int N = (int)colors.size() - 1;
    int inf = (int)(t * ((double)N));
    int sup = inf + 1;

    double delta = 1.0 / ((double)N);
    double r = t - inf * delta;
    double p = r / delta;

    if (inf < 0) inf = 0;
    if (inf > N) inf = N;
    if (sup < 0) sup = 0;
    if (sup > N) sup = N;

    ColorRgb* a = colors.get(inf);
    ColorRgb* b = colors.get(sup);

    double cr = a->r() + (b->r() - a->r()) * p;
    double cg = a->g() + (b->g() - a->g()) * p;
    double cb = a->b() + (b->b() - a->b()) * p;

    return new ColorRgb(cr, cg, cb);
}

int RGBColorPalette::selectNearestIndexToRgb(const ColorRgb& c) const {
    double minDistance = 1e308;
    int index = 0;

    for (long int i = 0; i < colors.size(); i++) {
        if (colors.get(i) != nullptr) {
            double currentDistance = colors.get(i)->distance(c);
            if (currentDistance < minDistance) {
                minDistance = currentDistance;
                index = (int)i;
            }
        }
    }
    return index;
}
