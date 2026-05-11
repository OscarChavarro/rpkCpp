#ifndef TRIANGLE_CUBATURE_RULE__
#define TRIANGLE_CUBATURE_RULE__

#include "numericalAnalysis/CubatureRule.h"

class TriangleCubatureRule {
  private:
    static constexpr double D_4_6_W1 = 3.298552309659655e-1 / 3.0;
    static constexpr double D_4_6_A1 = 8.168475729804585e-1;
    static constexpr double D_4_6_B1 = 9.157621350977073e-2;
    static constexpr double D_4_6_C1 = D_4_6_B1;
    static constexpr double D_4_6_W2 = 6.701447690340345e-1 / 3.0;
    static constexpr double D_4_6_A2 = 1.081030181680702e-1;
    static constexpr double D_4_6_B2 = 4.459484909159649e-1;
    static constexpr double D_4_6_C2 = D_4_6_B2;

    static constexpr double D_5_7_R = 0.1012865073234563;
    static constexpr double D_5_7_S = 0.7974269853530873;
    static constexpr double D_5_7_T = 1.0 / 3.0;
    static constexpr double D_5_7_U = 0.4701420641051151;
    static constexpr double D_5_7_V = 0.05971587178976981;
    static constexpr double D_5_7_A = 0.225;
    static constexpr double D_5_7_B = 0.1259391805448271;
    static constexpr double D_5_7_C = 0.1323941527885062;

    static constexpr double D_7_12_W1 = 0.2651702815743450e-1 * 2.0;
    static constexpr double D_7_12_A1 = 0.6238226509439084e-1;
    static constexpr double D_7_12_B1 = 0.6751786707392436e-1;
    static constexpr double D_7_12_C1 = 0.8700998678316848;
    static constexpr double D_7_12_W2 = 0.4388140871444811e-1 * 2.0;
    static constexpr double D_7_12_A2 = 0.5522545665692000e-1;
    static constexpr double D_7_12_B2 = 0.3215024938520156;
    static constexpr double D_7_12_C2 = 0.6232720494910644;
    static constexpr double D_7_12_W3 = 0.2877504278497528e-1 * 2.0;
    static constexpr double D_7_12_A3 = 0.3432430294509488e-1;
    static constexpr double D_7_12_B3 = 0.6609491961867980;
    static constexpr double D_7_12_C3 = 0.3047265008681072;
    static constexpr double D_7_12_W4 = 0.6749318700980879e-1 * 2.0;
    static constexpr double D_7_12_A4 = 0.5158423343536001;
    static constexpr double D_7_12_B4 = 0.2777161669764050;
    static constexpr double D_7_12_C4 = 0.2064414986699949;

    static constexpr double D_8_16_W0 = 1.443156076777862e-1;
    static constexpr double D_8_16_A0 = 3.333333333333333e-1;
    static constexpr double D_8_16_B0 = 3.333333333333333e-1;
    static constexpr double D_8_16_W1 = 2.852749028018549e-1 / 3.0;
    static constexpr double D_8_16_A1 = 8.141482341455413e-2;
    static constexpr double D_8_16_B1 = 4.592925882927229e-1;
    static constexpr double D_8_16_C1 = D_8_16_B1;
    static constexpr double D_8_16_W2 = 9.737549286959440e-2 / 3.0;
    static constexpr double D_8_16_A2 = 8.989055433659379e-1;
    static constexpr double D_8_16_B2 = 5.054722831703103e-2;
    static constexpr double D_8_16_C2 = D_8_16_B2;
    static constexpr double D_8_16_W3 = 3.096521116041552e-1 / 3.0;
    static constexpr double D_8_16_A3 = 6.588613844964797e-1;
    static constexpr double D_8_16_B3 = 1.705693077517601e-1;
    static constexpr double D_8_16_C3 = D_8_16_B3;
    static constexpr double D_8_16_W4 = 1.633818850466092e-1 / 6.0;
    static constexpr double D_8_16_A4 = 8.394777409957211e-3;
    static constexpr double D_8_16_B4 = 7.284923929554041e-1;
    static constexpr double D_8_16_C4 = 2.631128296346387e-1;

    static constexpr double D_9_19_W0 = 9.713579628279610e-2;
    static constexpr double D_9_19_A0 = 3.333333333333333e-1;
    static constexpr double D_9_19_B0 = 3.333333333333333e-1;
    static constexpr double D_9_19_W1 = 9.400410068141950e-2 / 3.0;
    static constexpr double D_9_19_A1 = 2.063496160252593e-2;
    static constexpr double D_9_19_B1 = 4.896825191987370e-1;
    static constexpr double D_9_19_C1 = D_9_19_B1;
    static constexpr double D_9_19_W2 = 2.334826230143263e-1 / 3.0;
    static constexpr double D_9_19_A2 = 1.258208170141290e-1;
    static constexpr double D_9_19_B2 = 4.370895914929355e-1;
    static constexpr double D_9_19_C2 = D_9_19_B2;
    static constexpr double D_9_19_W3 = 2.389432167816273e-1 / 3.0;
    static constexpr double D_9_19_A3 = 6.235929287619356e-1;
    static constexpr double D_9_19_B3 = 1.882035356190322e-1;
    static constexpr double D_9_19_C3 = D_9_19_B3;
    static constexpr double D_9_19_W4 = 7.673302697609430e-2 / 3.0;
    static constexpr double D_9_19_A4 = 9.105409732110941e-1;
    static constexpr double D_9_19_B4 = 4.472951339445297e-2;
    static constexpr double D_9_19_C4 = D_9_19_B4;
    static constexpr double D_9_19_W5 = 2.597012362637364e-1 / 6.0;
    static constexpr double D_9_19_A5 = 3.683841205473626e-2;
    static constexpr double D_9_19_B5 = 7.411985987844980e-1;
    static constexpr double D_9_19_C5 = 2.219629891607657e-1;

    static CubatureRule crt1;
    static CubatureRule crt2;
    static CubatureRule crt3;
    static CubatureRule crt4;
    static CubatureRule crt5;
    static CubatureRule crt7;
    static CubatureRule crt9;

  public:
    static CubatureRule *degree8Rule();
    static void setTriangleCubatureRules(CubatureRule **triRule, CubatureDegree degree);
};

#endif
