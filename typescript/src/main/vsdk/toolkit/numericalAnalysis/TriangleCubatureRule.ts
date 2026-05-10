import { Logger as VsdkLogger } from "../common/logging/Logger";
import { CubatureDegree } from "./CubatureDegree";
import { CubatureRule } from "./CubatureRule";

export class TriangleCubatureRule {
  private static readonly D_4_6_W1 = 3.298552309659655e-1 / 3.0;
  private static readonly D_4_6_A1 = 8.168475729804585e-1;
  private static readonly D_4_6_B1 = 9.157621350977073e-2;
  private static readonly D_4_6_C1 = TriangleCubatureRule.D_4_6_B1;
  private static readonly D_4_6_W2 = 6.701447690340345e-1 / 3.0;
  private static readonly D_4_6_A2 = 1.081030181680702e-1;
  private static readonly D_4_6_B2 = 4.459484909159649e-1;
  private static readonly D_4_6_C2 = TriangleCubatureRule.D_4_6_B2;

  private static readonly D_5_7_R = 0.1012865073234563;
  private static readonly D_5_7_S = 0.7974269853530873;
  private static readonly D_5_7_T = 1.0 / 3.0;
  private static readonly D_5_7_U = 0.4701420641051151;
  private static readonly D_5_7_V = 0.05971587178976981;
  private static readonly D_5_7_A = 0.225;
  private static readonly D_5_7_B = 0.1259391805448271;
  private static readonly D_5_7_C = 0.1323941527885062;

  private static readonly D_7_12_W1 = 0.2651702815743450e-1 * 2.0;
  private static readonly D_7_12_A1 = 0.6238226509439084e-1;
  private static readonly D_7_12_B1 = 0.6751786707392436e-1;
  private static readonly D_7_12_C1 = 0.8700998678316848;
  private static readonly D_7_12_W2 = 0.4388140871444811e-1 * 2.0;
  private static readonly D_7_12_A2 = 0.5522545665692000e-1;
  private static readonly D_7_12_B2 = 0.3215024938520156;
  private static readonly D_7_12_C2 = 0.6232720494910644;
  private static readonly D_7_12_W3 = 0.2877504278497528e-1 * 2.0;
  private static readonly D_7_12_A3 = 0.3432430294509488e-1;
  private static readonly D_7_12_B3 = 0.6609491961867980;
  private static readonly D_7_12_C3 = 0.3047265008681072;
  private static readonly D_7_12_W4 = 0.6749318700980879e-1 * 2.0;
  private static readonly D_7_12_A4 = 0.5158423343536001;
  private static readonly D_7_12_B4 = 0.2777161669764050;
  private static readonly D_7_12_C4 = 0.2064414986699949;

  private static readonly D_8_16_W0 = 1.443156076777862e-1;
  private static readonly D_8_16_A0 = 3.333333333333333e-1;
  private static readonly D_8_16_B0 = 3.333333333333333e-1;
  private static readonly D_8_16_W1 = 2.852749028018549e-1 / 3.0;
  private static readonly D_8_16_A1 = 8.141482341455413e-2;
  private static readonly D_8_16_B1 = 4.592925882927229e-1;
  private static readonly D_8_16_C1 = TriangleCubatureRule.D_8_16_B1;
  private static readonly D_8_16_W2 = 9.737549286959440e-2 / 3.0;
  private static readonly D_8_16_A2 = 8.989055433659379e-1;
  private static readonly D_8_16_B2 = 5.054722831703103e-2;
  private static readonly D_8_16_C2 = TriangleCubatureRule.D_8_16_B2;
  private static readonly D_8_16_W3 = 3.096521116041552e-1 / 3.0;
  private static readonly D_8_16_A3 = 6.588613844964797e-1;
  private static readonly D_8_16_B3 = 1.705693077517601e-1;
  private static readonly D_8_16_C3 = TriangleCubatureRule.D_8_16_B3;
  private static readonly D_8_16_W4 = 1.633818850466092e-1 / 6.0;
  private static readonly D_8_16_A4 = 8.394777409957211e-3;
  private static readonly D_8_16_B4 = 7.284923929554041e-1;
  private static readonly D_8_16_C4 = 2.631128296346387e-1;

  private static readonly D_9_19_W0 = 9.713579628279610e-2;
  private static readonly D_9_19_A0 = 3.333333333333333e-1;
  private static readonly D_9_19_B0 = 3.333333333333333e-1;
  private static readonly D_9_19_W1 = 9.400410068141950e-2 / 3.0;
  private static readonly D_9_19_A1 = 2.063496160252593e-2;
  private static readonly D_9_19_B1 = 4.896825191987370e-1;
  private static readonly D_9_19_C1 = TriangleCubatureRule.D_9_19_B1;
  private static readonly D_9_19_W2 = 2.334826230143263e-1 / 3.0;
  private static readonly D_9_19_A2 = 1.258208170141290e-1;
  private static readonly D_9_19_B2 = 4.370895914929355e-1;
  private static readonly D_9_19_C2 = TriangleCubatureRule.D_9_19_B2;
  private static readonly D_9_19_W3 = 2.389432167816273e-1 / 3.0;
  private static readonly D_9_19_A3 = 6.235929287619356e-1;
  private static readonly D_9_19_B3 = 1.882035356190322e-1;
  private static readonly D_9_19_C3 = TriangleCubatureRule.D_9_19_B3;
  private static readonly D_9_19_W4 = 7.673302697609430e-2 / 3.0;
  private static readonly D_9_19_A4 = 9.105409732110941e-1;
  private static readonly D_9_19_B4 = 4.472951339445297e-2;
  private static readonly D_9_19_C4 = TriangleCubatureRule.D_9_19_B4;
  private static readonly D_9_19_W5 = 2.597012362637364e-1 / 6.0;
  private static readonly D_9_19_A5 = 3.683841205473626e-2;
  private static readonly D_9_19_B5 = 7.411985987844980e-1;
  private static readonly D_9_19_C5 = 2.219629891607657e-1;

  private static readonly crt1 = TriangleCubatureRule.createRule(
    "triangles degree 1, 1 positions",
    1,
    [1.0 / 3.0],
    [1.0 / 3.0],
    [0.0],
    [1.0]
  );

  private static readonly crt2 = TriangleCubatureRule.createRule(
    "triangles degree 2, 3 positions",
    3,
    [1.0 / 6.0, 1.0 / 6.0, 2.0 / 3.0],
    [1.0 / 6.0, 2.0 / 3.0, 1.0 / 6.0],
    [0.0, 0.0, 0.0],
    [1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0]
  );

  private static readonly crt3 = TriangleCubatureRule.createRule(
    "triangles degree 3, 4 positions",
    4,
    [1.0 / 3.0, 0.2, 0.2, 0.6],
    [1.0 / 3.0, 0.2, 0.6, 0.2],
    [0.0, 0.0, 0.0, 0.0],
    [-9.0 / 16.0, 25.0 / 48.0, 25.0 / 48.0, 25.0 / 48.0]
  );

  private static readonly crt4 = TriangleCubatureRule.createRule(
    "triangles degree 4, 6 positions",
    6,
    [
      TriangleCubatureRule.D_4_6_A1,
      TriangleCubatureRule.D_4_6_B1,
      TriangleCubatureRule.D_4_6_C1,
      TriangleCubatureRule.D_4_6_A2,
      TriangleCubatureRule.D_4_6_B2,
      TriangleCubatureRule.D_4_6_C2
    ],
    [
      TriangleCubatureRule.D_4_6_B1,
      TriangleCubatureRule.D_4_6_C1,
      TriangleCubatureRule.D_4_6_A1,
      TriangleCubatureRule.D_4_6_B2,
      TriangleCubatureRule.D_4_6_C2,
      TriangleCubatureRule.D_4_6_A2
    ],
    [0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
    [
      TriangleCubatureRule.D_4_6_W1,
      TriangleCubatureRule.D_4_6_W1,
      TriangleCubatureRule.D_4_6_W1,
      TriangleCubatureRule.D_4_6_W2,
      TriangleCubatureRule.D_4_6_W2,
      TriangleCubatureRule.D_4_6_W2
    ]
  );

  private static readonly crt5 = TriangleCubatureRule.createRule(
    "triangles degree 5, 7 positions",
    7,
    [
      TriangleCubatureRule.D_5_7_T,
      TriangleCubatureRule.D_5_7_R,
      TriangleCubatureRule.D_5_7_R,
      TriangleCubatureRule.D_5_7_S,
      TriangleCubatureRule.D_5_7_U,
      TriangleCubatureRule.D_5_7_U,
      TriangleCubatureRule.D_5_7_V
    ],
    [
      TriangleCubatureRule.D_5_7_T,
      TriangleCubatureRule.D_5_7_R,
      TriangleCubatureRule.D_5_7_S,
      TriangleCubatureRule.D_5_7_R,
      TriangleCubatureRule.D_5_7_U,
      TriangleCubatureRule.D_5_7_V,
      TriangleCubatureRule.D_5_7_U
    ],
    [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
    [
      TriangleCubatureRule.D_5_7_A,
      TriangleCubatureRule.D_5_7_B,
      TriangleCubatureRule.D_5_7_B,
      TriangleCubatureRule.D_5_7_B,
      TriangleCubatureRule.D_5_7_C,
      TriangleCubatureRule.D_5_7_C,
      TriangleCubatureRule.D_5_7_C
    ]
  );

  private static readonly crt7 = TriangleCubatureRule.createRule(
    "triangles degree 7, 12 positions",
    12,
    [
      TriangleCubatureRule.D_7_12_A1, TriangleCubatureRule.D_7_12_B1, TriangleCubatureRule.D_7_12_C1,
      TriangleCubatureRule.D_7_12_A2, TriangleCubatureRule.D_7_12_B2, TriangleCubatureRule.D_7_12_C2,
      TriangleCubatureRule.D_7_12_A3, TriangleCubatureRule.D_7_12_B3, TriangleCubatureRule.D_7_12_C3,
      TriangleCubatureRule.D_7_12_A4, TriangleCubatureRule.D_7_12_B4, TriangleCubatureRule.D_7_12_C4
    ],
    [
      TriangleCubatureRule.D_7_12_B1, TriangleCubatureRule.D_7_12_C1, TriangleCubatureRule.D_7_12_A1,
      TriangleCubatureRule.D_7_12_B2, TriangleCubatureRule.D_7_12_C2, TriangleCubatureRule.D_7_12_A2,
      TriangleCubatureRule.D_7_12_B3, TriangleCubatureRule.D_7_12_C3, TriangleCubatureRule.D_7_12_A3,
      TriangleCubatureRule.D_7_12_B4, TriangleCubatureRule.D_7_12_C4, TriangleCubatureRule.D_7_12_A4
    ],
    [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
    [
      TriangleCubatureRule.D_7_12_W1, TriangleCubatureRule.D_7_12_W1, TriangleCubatureRule.D_7_12_W1,
      TriangleCubatureRule.D_7_12_W2, TriangleCubatureRule.D_7_12_W2, TriangleCubatureRule.D_7_12_W2,
      TriangleCubatureRule.D_7_12_W3, TriangleCubatureRule.D_7_12_W3, TriangleCubatureRule.D_7_12_W3,
      TriangleCubatureRule.D_7_12_W4, TriangleCubatureRule.D_7_12_W4, TriangleCubatureRule.D_7_12_W4
    ]
  );

  private static readonly crt9 = TriangleCubatureRule.createRule(
    "triangles degree 9, 19 positions",
    19,
    [
      TriangleCubatureRule.D_9_19_A0,
      TriangleCubatureRule.D_9_19_A1, TriangleCubatureRule.D_9_19_B1, TriangleCubatureRule.D_9_19_C1,
      TriangleCubatureRule.D_9_19_A2, TriangleCubatureRule.D_9_19_B2, TriangleCubatureRule.D_9_19_C2,
      TriangleCubatureRule.D_9_19_A3, TriangleCubatureRule.D_9_19_B3, TriangleCubatureRule.D_9_19_C3,
      TriangleCubatureRule.D_9_19_A4, TriangleCubatureRule.D_9_19_B4, TriangleCubatureRule.D_9_19_C4,
      TriangleCubatureRule.D_9_19_A5, TriangleCubatureRule.D_9_19_A5,
      TriangleCubatureRule.D_9_19_B5, TriangleCubatureRule.D_9_19_B5,
      TriangleCubatureRule.D_9_19_C5, TriangleCubatureRule.D_9_19_C5
    ],
    [
      TriangleCubatureRule.D_9_19_B0,
      TriangleCubatureRule.D_9_19_B1, TriangleCubatureRule.D_9_19_C1, TriangleCubatureRule.D_9_19_A1,
      TriangleCubatureRule.D_9_19_B2, TriangleCubatureRule.D_9_19_C2, TriangleCubatureRule.D_9_19_A2,
      TriangleCubatureRule.D_9_19_B3, TriangleCubatureRule.D_9_19_C3, TriangleCubatureRule.D_9_19_A3,
      TriangleCubatureRule.D_9_19_B4, TriangleCubatureRule.D_9_19_C4, TriangleCubatureRule.D_9_19_A4,
      TriangleCubatureRule.D_9_19_B5, TriangleCubatureRule.D_9_19_C5,
      TriangleCubatureRule.D_9_19_C5, TriangleCubatureRule.D_9_19_A5,
      TriangleCubatureRule.D_9_19_A5, TriangleCubatureRule.D_9_19_B5
    ],
    [
      0.0,
      0.0, 0.0, 0.0,
      0.0, 0.0, 0.0,
      0.0, 0.0, 0.0,
      0.0, 0.0, 0.0,
      0.0, 0.0, 0.0, 0.0, 0.0, 0.0
    ],
    [
      TriangleCubatureRule.D_9_19_W0,
      TriangleCubatureRule.D_9_19_W1, TriangleCubatureRule.D_9_19_W1, TriangleCubatureRule.D_9_19_W1,
      TriangleCubatureRule.D_9_19_W2, TriangleCubatureRule.D_9_19_W2, TriangleCubatureRule.D_9_19_W2,
      TriangleCubatureRule.D_9_19_W3, TriangleCubatureRule.D_9_19_W3, TriangleCubatureRule.D_9_19_W3,
      TriangleCubatureRule.D_9_19_W4, TriangleCubatureRule.D_9_19_W4, TriangleCubatureRule.D_9_19_W4,
      TriangleCubatureRule.D_9_19_W5, TriangleCubatureRule.D_9_19_W5, TriangleCubatureRule.D_9_19_W5,
      TriangleCubatureRule.D_9_19_W5, TriangleCubatureRule.D_9_19_W5, TriangleCubatureRule.D_9_19_W5
    ]
  );

  private static degree8RuleInstance: CubatureRule | null = null;

  private static createRule(
    description: string,
    numberOfNodes: number,
    u: number[],
    v: number[],
    t: number[],
    w: number[]
  ): CubatureRule {
    return new CubatureRule(description, numberOfNodes, u, v, t, w);
  }

  public static degree8Rule(): CubatureRule {
    if (TriangleCubatureRule.degree8RuleInstance === null) {
      TriangleCubatureRule.degree8RuleInstance = TriangleCubatureRule.createRule(
        "triangles degree 8, 16 positions",
        16,
        [
          TriangleCubatureRule.D_8_16_A0,
          TriangleCubatureRule.D_8_16_A1, TriangleCubatureRule.D_8_16_B1, TriangleCubatureRule.D_8_16_C1,
          TriangleCubatureRule.D_8_16_A2, TriangleCubatureRule.D_8_16_B2, TriangleCubatureRule.D_8_16_C2,
          TriangleCubatureRule.D_8_16_A3, TriangleCubatureRule.D_8_16_B3, TriangleCubatureRule.D_8_16_C3,
          TriangleCubatureRule.D_8_16_A4, TriangleCubatureRule.D_8_16_A4,
          TriangleCubatureRule.D_8_16_B4, TriangleCubatureRule.D_8_16_B4,
          TriangleCubatureRule.D_8_16_C4, TriangleCubatureRule.D_8_16_C4
        ],
        [
          TriangleCubatureRule.D_8_16_B0,
          TriangleCubatureRule.D_8_16_B1, TriangleCubatureRule.D_8_16_C1, TriangleCubatureRule.D_8_16_A1,
          TriangleCubatureRule.D_8_16_B2, TriangleCubatureRule.D_8_16_C2, TriangleCubatureRule.D_8_16_A2,
          TriangleCubatureRule.D_8_16_B3, TriangleCubatureRule.D_8_16_C3, TriangleCubatureRule.D_8_16_A3,
          TriangleCubatureRule.D_8_16_B4, TriangleCubatureRule.D_8_16_C4,
          TriangleCubatureRule.D_8_16_C4, TriangleCubatureRule.D_8_16_A4,
          TriangleCubatureRule.D_8_16_A4, TriangleCubatureRule.D_8_16_B4
        ],
        [
          0.0,
          0.0, 0.0, 0.0,
          0.0, 0.0, 0.0,
          0.0, 0.0, 0.0,
          0.0, 0.0, 0.0, 0.0, 0.0, 0.0
        ],
        [
          TriangleCubatureRule.D_8_16_W0,
          TriangleCubatureRule.D_8_16_W1, TriangleCubatureRule.D_8_16_W1, TriangleCubatureRule.D_8_16_W1,
          TriangleCubatureRule.D_8_16_W2, TriangleCubatureRule.D_8_16_W2, TriangleCubatureRule.D_8_16_W2,
          TriangleCubatureRule.D_8_16_W3, TriangleCubatureRule.D_8_16_W3, TriangleCubatureRule.D_8_16_W3,
          TriangleCubatureRule.D_8_16_W4, TriangleCubatureRule.D_8_16_W4, TriangleCubatureRule.D_8_16_W4,
          TriangleCubatureRule.D_8_16_W4, TriangleCubatureRule.D_8_16_W4, TriangleCubatureRule.D_8_16_W4
        ]
      );
    }
    return TriangleCubatureRule.degree8RuleInstance;
  }

  public static setTriangleCubatureRules(triRule: Array<CubatureRule | null>, degree: CubatureDegree): void {
    switch (degree) {
      case CubatureDegree.DEGREE_1:
        triRule[0] = TriangleCubatureRule.crt1;
        break;
      case CubatureDegree.DEGREE_2:
        triRule[0] = TriangleCubatureRule.crt2;
        break;
      case CubatureDegree.DEGREE_3:
        triRule[0] = TriangleCubatureRule.crt3;
        break;
      case CubatureDegree.DEGREE_4:
        triRule[0] = TriangleCubatureRule.crt4;
        break;
      case CubatureDegree.DEGREE_5:
        triRule[0] = TriangleCubatureRule.crt5;
        break;
      case CubatureDegree.DEGREE_6:
      case CubatureDegree.DEGREE_7:
        triRule[0] = TriangleCubatureRule.crt7;
        break;
      case CubatureDegree.DEGREE_8:
        triRule[0] = TriangleCubatureRule.degree8Rule();
        break;
      case CubatureDegree.DEGREE_9:
        triRule[0] = TriangleCubatureRule.crt9;
        break;
      case CubatureDegree.DEGREE_3_PROD:
        triRule[0] = TriangleCubatureRule.crt5;
        break;
      case CubatureDegree.DEGREE_5_PROD:
        triRule[0] = TriangleCubatureRule.crt7;
        break;
      case CubatureDegree.DEGREE_7_PROD:
        triRule[0] = TriangleCubatureRule.crt9;
        break;
      default:
        VsdkLogger.fatal(2, "setTriangleCubatureRules", "Invalid degree %d", degree);
    }
  }

  private constructor() {
  }
}
