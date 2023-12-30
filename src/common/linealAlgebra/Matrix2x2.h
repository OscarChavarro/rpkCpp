#ifndef __Transform2x2__
#define __Transform2x2__

/*
 * |u'|   |m[0][0]  m[0][1]|   | u |   |t[0]|
 * |  | = |                | * |   | + |    |
 * |v'|   |m[1][0]  m[1][1]|   | v |   |t[1]|
 */
class Matrix2x2 {
  public:
    float m[2][2];
    float t[2];
};

#endif
