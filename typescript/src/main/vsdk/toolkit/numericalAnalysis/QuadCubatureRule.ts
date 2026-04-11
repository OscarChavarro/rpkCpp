import { Error as VsdkError } from "../common/Error";
import { CubatureDegree } from "./CubatureDegree";
import { CubatureRule } from "./CubatureRule";

export class QuadCubatureRule {
  private static readonly D_2_3_W = 4.0 / 3.0;
  private static readonly D_2_3_U = 0.81649658092772603272;
  private static readonly D_2_3_C = -0.5;
  private static readonly D_2_3_S = 0.86602540378443864676;

  private static readonly D_3_4_U = 0.81649658092772603272;
  private static readonly D_3_4_G_U = 0.57735026918962576450;

  private static readonly D_5_7_W1 = 8.0 / 7.0;
  private static readonly D_5_7_W2 = 5.0 / 9.0;
  private static readonly D_5_7_W3 = 20.0 / 63.0;
  private static readonly D_5_7_R = 0.96609178307929588492;
  private static readonly D_5_7_S = 0.57735026918962573106;
  private static readonly D_5_7_T = 0.77459666924148340428;

  private static readonly D_5_9_X0 = 0.0;
  private static readonly D_5_9_W0 = 8.0 / 9.0;
  private static readonly D_5_9_X1 = 0.7745966692414834;
  private static readonly D_5_9_W1 = 5.0 / 9.0;

  private static readonly D_7_12_R = 0.92582009977255141919;
  private static readonly D_7_12_S = 0.38055443320831561227;
  private static readonly D_7_12_T = 0.80597978291859884159;
  private static readonly D_7_12_W1 = 0.24197530864197530631;
  private static readonly D_7_12_W2 = 0.52059291666739448967;
  private static readonly D_7_12_W3 = 0.23743177469063023177;

  private static readonly D_7_16_X1 = 0.86113631159405257522;
  private static readonly D_7_16_X2 = 0.33998104358485626480;
  private static readonly D_7_16_W1 = 0.34785484513745385737;
  private static readonly D_7_16_W2 = 0.65214515486254614263;

  private static readonly D_9_17_B1 = 0.96884996636197772072;
  private static readonly D_9_17_B2 = 0.75027709997890053354;
  private static readonly D_9_17_B3 = 0.52373582021442933604;
  private static readonly D_9_17_B4 = 0.07620832819261717318;
  private static readonly D_9_17_C1 = 0.63068011973166885417;
  private static readonly D_9_17_C2 = 0.92796164595956966740;
  private static readonly D_9_17_C3 = 0.45333982113564719076;
  private static readonly D_9_17_C4 = 0.85261572933366230775;
  private static readonly D_9_17_W0 = 0.52674897119341563786;
  private static readonly D_9_17_W1 = 0.08887937817019870697;
  private static readonly D_9_17_W2 = 0.11209960212959648528;
  private static readonly D_9_17_W3 = 0.39828243926207009528;
  private static readonly D_9_17_W4 = 0.26905133763978080301;

  private static readonly D_1_9_U = 1.0;
  private static readonly D_1_9_W = 8.0 / 9.0;
  private static readonly D_3_8_U = 0.57735026918962576450;

  private static crq1 = QuadCubatureRule.createTransformedQuadRule(new CubatureRule(
    "quads degree 1, 1 point",
    1,
    [0.0],
    [0.0],
    [0.0],
    [4.0]
  ));

  private static crq2 = QuadCubatureRule.createTransformedQuadRule(new CubatureRule(
    "quads degree 2, 3 positions",
    3,
    [
      QuadCubatureRule.D_2_3_U,
      QuadCubatureRule.D_2_3_U * QuadCubatureRule.D_2_3_C,
      QuadCubatureRule.D_2_3_U * QuadCubatureRule.D_2_3_C
    ],
    [
      0.0,
      QuadCubatureRule.D_2_3_U * QuadCubatureRule.D_2_3_S,
      -QuadCubatureRule.D_2_3_U * QuadCubatureRule.D_2_3_S
    ],
    [0.0, 0.0, 0.0],
    [QuadCubatureRule.D_2_3_W, QuadCubatureRule.D_2_3_W, QuadCubatureRule.D_2_3_W]
  ));

  private static crq3 = QuadCubatureRule.createTransformedQuadRule(new CubatureRule(
    "quads degree 3, 4 positions",
    4,
    [QuadCubatureRule.D_3_4_U, 0.0, -QuadCubatureRule.D_3_4_U, 0.0],
    [0.0, QuadCubatureRule.D_3_4_U, 0.0, -QuadCubatureRule.D_3_4_U],
    [0.0, 0.0, 0.0, 0.0],
    [1.0, 1.0, 1.0, 1.0]
  ));

  private static crq3Pg = QuadCubatureRule.createTransformedQuadRule(new CubatureRule(
    "quads degree 3, 4 positions, product Gauss formula",
    4,
    [
      QuadCubatureRule.D_3_4_G_U,
      QuadCubatureRule.D_3_4_G_U,
      -QuadCubatureRule.D_3_4_G_U,
      -QuadCubatureRule.D_3_4_G_U
    ],
    [
      QuadCubatureRule.D_3_4_G_U,
      -QuadCubatureRule.D_3_4_G_U,
      QuadCubatureRule.D_3_4_G_U,
      -QuadCubatureRule.D_3_4_G_U
    ],
    [0.0, 0.0, 0.0, 0.0],
    [1.0, 1.0, 1.0, 1.0]
  ));

  private static crq4 = QuadCubatureRule.createTransformedQuadRule(new CubatureRule(
    "quads degree 4, 6 positions",
    6,
    [
      0.0, 0.0, 0.774596669241483, -0.774596669241483,
      0.774596669241483, -0.774596669241483
    ],
    [
      -0.356822089773090, 0.934172358962716, 0.390885162530071,
      0.390885162530071, -0.852765377881771, -0.852765377881771
    ],
    [0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
    [
      1.286412084888852, 0.491365692888926, 0.761883709085613,
      0.761883709085613, 0.349227402025498, 0.349227402025498
    ]
  ));

  private static crq5 = QuadCubatureRule.createTransformedQuadRule(new CubatureRule(
    "quads degree 5, 7 positions, Radon's rule",
    7,
    [
      0.0,
      QuadCubatureRule.D_5_7_S,
      QuadCubatureRule.D_5_7_S,
      -QuadCubatureRule.D_5_7_S,
      -QuadCubatureRule.D_5_7_S,
      QuadCubatureRule.D_5_7_R,
      -QuadCubatureRule.D_5_7_R
    ],
    [
      0.0,
      QuadCubatureRule.D_5_7_T,
      -QuadCubatureRule.D_5_7_T,
      QuadCubatureRule.D_5_7_T,
      -QuadCubatureRule.D_5_7_T,
      0.0,
      0.0
    ],
    [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
    [
      QuadCubatureRule.D_5_7_W1,
      QuadCubatureRule.D_5_7_W2,
      QuadCubatureRule.D_5_7_W2,
      QuadCubatureRule.D_5_7_W2,
      QuadCubatureRule.D_5_7_W2,
      QuadCubatureRule.D_5_7_W3,
      QuadCubatureRule.D_5_7_W3
    ]
  ));

  private static crq5Pg = QuadCubatureRule.createTransformedQuadRule(new CubatureRule(
    "quads degree 5, 9 positions product Gauss rule",
    9,
    [
      -QuadCubatureRule.D_5_9_X1, -QuadCubatureRule.D_5_9_X1, -QuadCubatureRule.D_5_9_X1,
      QuadCubatureRule.D_5_9_X0, QuadCubatureRule.D_5_9_X0, QuadCubatureRule.D_5_9_X0,
      QuadCubatureRule.D_5_9_X1, QuadCubatureRule.D_5_9_X1, QuadCubatureRule.D_5_9_X1
    ],
    [
      -QuadCubatureRule.D_5_9_X1, QuadCubatureRule.D_5_9_X0, QuadCubatureRule.D_5_9_X1,
      -QuadCubatureRule.D_5_9_X1, QuadCubatureRule.D_5_9_X0, QuadCubatureRule.D_5_9_X1,
      -QuadCubatureRule.D_5_9_X1, QuadCubatureRule.D_5_9_X0, QuadCubatureRule.D_5_9_X1
    ],
    [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
    [
      QuadCubatureRule.D_5_9_W1 * QuadCubatureRule.D_5_9_W1,
      QuadCubatureRule.D_5_9_W1 * QuadCubatureRule.D_5_9_W0,
      QuadCubatureRule.D_5_9_W1 * QuadCubatureRule.D_5_9_W1,
      QuadCubatureRule.D_5_9_W0 * QuadCubatureRule.D_5_9_W1,
      QuadCubatureRule.D_5_9_W0 * QuadCubatureRule.D_5_9_W0,
      QuadCubatureRule.D_5_9_W0 * QuadCubatureRule.D_5_9_W1,
      QuadCubatureRule.D_5_9_W1 * QuadCubatureRule.D_5_9_W1,
      QuadCubatureRule.D_5_9_W1 * QuadCubatureRule.D_5_9_W0,
      QuadCubatureRule.D_5_9_W1 * QuadCubatureRule.D_5_9_W1
    ]
  ));

  private static crq6 = QuadCubatureRule.createTransformedQuadRule(new CubatureRule(
    "quads degree 6, 10 positions",
    10,
    [
      0.0, 0.0, 0.863742826346154, -0.863742826346154,
      0.518690521392592, -0.518690521392592, 0.933972544972849, -0.933972544972849,
      0.608977536016356, -0.608977536016356
    ],
    [
      0.869833375250059, -0.479406351612111, 0.802837516207657, 0.802837516207657,
      0.262143665508058, 0.262143665508058, -0.363096583148066, -0.363096583148066,
      -0.896608632762453, -0.896608632762453
    ],
    [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
    [
      0.392750590964348, 0.754762881242610, 0.206166050588279, 0.206166050588279,
      0.689992138489864, 0.689992138489864, 0.260517488732317, 0.260517488732317,
      0.269567586086061, 0.269567586086061
    ]
  ));

  private static crq7 = QuadCubatureRule.createTransformedQuadRule(new CubatureRule(
    "quads degree 7, 12 positions",
    12,
    [
      QuadCubatureRule.D_7_12_R, -QuadCubatureRule.D_7_12_R, 0.0, 0.0,
      QuadCubatureRule.D_7_12_S, QuadCubatureRule.D_7_12_S,
      -QuadCubatureRule.D_7_12_S, -QuadCubatureRule.D_7_12_S,
      QuadCubatureRule.D_7_12_T, QuadCubatureRule.D_7_12_T,
      -QuadCubatureRule.D_7_12_T, -QuadCubatureRule.D_7_12_T
    ],
    [
      0.0, 0.0, QuadCubatureRule.D_7_12_R, -QuadCubatureRule.D_7_12_R,
      QuadCubatureRule.D_7_12_S, -QuadCubatureRule.D_7_12_S,
      QuadCubatureRule.D_7_12_S, -QuadCubatureRule.D_7_12_S,
      QuadCubatureRule.D_7_12_T, -QuadCubatureRule.D_7_12_T,
      QuadCubatureRule.D_7_12_T, -QuadCubatureRule.D_7_12_T
    ],
    [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
    [
      QuadCubatureRule.D_7_12_W1, QuadCubatureRule.D_7_12_W1,
      QuadCubatureRule.D_7_12_W1, QuadCubatureRule.D_7_12_W1,
      QuadCubatureRule.D_7_12_W2, QuadCubatureRule.D_7_12_W2,
      QuadCubatureRule.D_7_12_W2, QuadCubatureRule.D_7_12_W2,
      QuadCubatureRule.D_7_12_W3, QuadCubatureRule.D_7_12_W3,
      QuadCubatureRule.D_7_12_W3, QuadCubatureRule.D_7_12_W3
    ]
  ));

  private static crq7Pg = QuadCubatureRule.createTransformedQuadRule(new CubatureRule(
    "quads degree 7, 16 positions product Gauss rule",
    16,
    [
      -QuadCubatureRule.D_7_16_X1, -QuadCubatureRule.D_7_16_X1,
      -QuadCubatureRule.D_7_16_X1, -QuadCubatureRule.D_7_16_X1,
      -QuadCubatureRule.D_7_16_X2, -QuadCubatureRule.D_7_16_X2,
      -QuadCubatureRule.D_7_16_X2, -QuadCubatureRule.D_7_16_X2,
      QuadCubatureRule.D_7_16_X2, QuadCubatureRule.D_7_16_X2,
      QuadCubatureRule.D_7_16_X2, QuadCubatureRule.D_7_16_X2,
      QuadCubatureRule.D_7_16_X1, QuadCubatureRule.D_7_16_X1,
      QuadCubatureRule.D_7_16_X1, QuadCubatureRule.D_7_16_X1
    ],
    [
      -QuadCubatureRule.D_7_16_X1, -QuadCubatureRule.D_7_16_X2,
      QuadCubatureRule.D_7_16_X2, QuadCubatureRule.D_7_16_X1,
      -QuadCubatureRule.D_7_16_X1, -QuadCubatureRule.D_7_16_X2,
      QuadCubatureRule.D_7_16_X2, QuadCubatureRule.D_7_16_X1,
      -QuadCubatureRule.D_7_16_X1, -QuadCubatureRule.D_7_16_X2,
      QuadCubatureRule.D_7_16_X2, QuadCubatureRule.D_7_16_X1,
      -QuadCubatureRule.D_7_16_X1, -QuadCubatureRule.D_7_16_X2,
      QuadCubatureRule.D_7_16_X2, QuadCubatureRule.D_7_16_X1
    ],
    [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
    [
      QuadCubatureRule.D_7_16_W1 * QuadCubatureRule.D_7_16_W1,
      QuadCubatureRule.D_7_16_W1 * QuadCubatureRule.D_7_16_W2,
      QuadCubatureRule.D_7_16_W1 * QuadCubatureRule.D_7_16_W2,
      QuadCubatureRule.D_7_16_W1 * QuadCubatureRule.D_7_16_W1,
      QuadCubatureRule.D_7_16_W2 * QuadCubatureRule.D_7_16_W1,
      QuadCubatureRule.D_7_16_W2 * QuadCubatureRule.D_7_16_W2,
      QuadCubatureRule.D_7_16_W2 * QuadCubatureRule.D_7_16_W2,
      QuadCubatureRule.D_7_16_W2 * QuadCubatureRule.D_7_16_W1,
      QuadCubatureRule.D_7_16_W2 * QuadCubatureRule.D_7_16_W1,
      QuadCubatureRule.D_7_16_W2 * QuadCubatureRule.D_7_16_W2,
      QuadCubatureRule.D_7_16_W2 * QuadCubatureRule.D_7_16_W2,
      QuadCubatureRule.D_7_16_W2 * QuadCubatureRule.D_7_16_W1,
      QuadCubatureRule.D_7_16_W1 * QuadCubatureRule.D_7_16_W1,
      QuadCubatureRule.D_7_16_W1 * QuadCubatureRule.D_7_16_W2,
      QuadCubatureRule.D_7_16_W1 * QuadCubatureRule.D_7_16_W2,
      QuadCubatureRule.D_7_16_W1 * QuadCubatureRule.D_7_16_W1
    ]
  ));

  private static crq9 = QuadCubatureRule.createTransformedQuadRule(new CubatureRule(
    "quads degree 9, 17 positions",
    17,
    [
      0.0,
      QuadCubatureRule.D_9_17_B1, -QuadCubatureRule.D_9_17_B1, -QuadCubatureRule.D_9_17_C1, QuadCubatureRule.D_9_17_C1,
      QuadCubatureRule.D_9_17_B2, -QuadCubatureRule.D_9_17_B2, -QuadCubatureRule.D_9_17_C2, QuadCubatureRule.D_9_17_C2,
      QuadCubatureRule.D_9_17_B3, -QuadCubatureRule.D_9_17_B3, -QuadCubatureRule.D_9_17_C3, QuadCubatureRule.D_9_17_C3,
      QuadCubatureRule.D_9_17_B4, -QuadCubatureRule.D_9_17_B4, -QuadCubatureRule.D_9_17_C4, QuadCubatureRule.D_9_17_C4
    ],
    [
      0.0,
      QuadCubatureRule.D_9_17_C1, -QuadCubatureRule.D_9_17_C1, QuadCubatureRule.D_9_17_B1, -QuadCubatureRule.D_9_17_B1,
      QuadCubatureRule.D_9_17_C2, -QuadCubatureRule.D_9_17_C2, QuadCubatureRule.D_9_17_B2, -QuadCubatureRule.D_9_17_B2,
      QuadCubatureRule.D_9_17_C3, -QuadCubatureRule.D_9_17_C3, QuadCubatureRule.D_9_17_B3, -QuadCubatureRule.D_9_17_B3,
      QuadCubatureRule.D_9_17_C4, -QuadCubatureRule.D_9_17_C4, QuadCubatureRule.D_9_17_B4, -QuadCubatureRule.D_9_17_B4
    ],
    [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
    [
      QuadCubatureRule.D_9_17_W0,
      QuadCubatureRule.D_9_17_W1, QuadCubatureRule.D_9_17_W1, QuadCubatureRule.D_9_17_W1, QuadCubatureRule.D_9_17_W1,
      QuadCubatureRule.D_9_17_W2, QuadCubatureRule.D_9_17_W2, QuadCubatureRule.D_9_17_W2, QuadCubatureRule.D_9_17_W2,
      QuadCubatureRule.D_9_17_W3, QuadCubatureRule.D_9_17_W3, QuadCubatureRule.D_9_17_W3, QuadCubatureRule.D_9_17_W3,
      QuadCubatureRule.D_9_17_W4, QuadCubatureRule.D_9_17_W4, QuadCubatureRule.D_9_17_W4, QuadCubatureRule.D_9_17_W4
    ]
  ));

  private static crv3Pg = QuadCubatureRule.createTransformedCubeRule(new CubatureRule(
    "boxes degree 3, 8 positions, product Gauss formula",
    8,
    [
      QuadCubatureRule.D_3_8_U, QuadCubatureRule.D_3_8_U, QuadCubatureRule.D_3_8_U, QuadCubatureRule.D_3_8_U,
      -QuadCubatureRule.D_3_8_U, -QuadCubatureRule.D_3_8_U, -QuadCubatureRule.D_3_8_U, -QuadCubatureRule.D_3_8_U
    ],
    [
      QuadCubatureRule.D_3_8_U, QuadCubatureRule.D_3_8_U, -QuadCubatureRule.D_3_8_U, -QuadCubatureRule.D_3_8_U,
      QuadCubatureRule.D_3_8_U, QuadCubatureRule.D_3_8_U, -QuadCubatureRule.D_3_8_U, -QuadCubatureRule.D_3_8_U
    ],
    [
      QuadCubatureRule.D_3_8_U, -QuadCubatureRule.D_3_8_U, QuadCubatureRule.D_3_8_U, -QuadCubatureRule.D_3_8_U,
      QuadCubatureRule.D_3_8_U, -QuadCubatureRule.D_3_8_U, QuadCubatureRule.D_3_8_U, -QuadCubatureRule.D_3_8_U
    ],
    [1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0]
  ));

  private static readonly quadProductRule: CubatureRule[] = [
    QuadCubatureRule.crq3Pg,
    QuadCubatureRule.crq5Pg,
    QuadCubatureRule.crq7Pg
  ];

  private static readonly boxesProductRule: CubatureRule[] = [QuadCubatureRule.crv3Pg];

  private static degree8QuadrilateralRuleInstance: CubatureRule | null = null;
  private static degree1BoxRuleInstance: CubatureRule | null = null;

  private static transformQuadRuleInPlace(rule: CubatureRule | null): void {
    if (rule === null) {
      return;
    }
    for (let k = 0; k < rule.numberOfNodes; k++) {
      rule.u[k] = (rule.u[k] + 1.0) / 2.0;
      rule.v[k] = (rule.v[k] + 1.0) / 2.0;
      rule.w[k] /= 4.0;
    }
  }

  private static transformCubeRuleInPlace(rule: CubatureRule | null): void {
    if (rule === null) {
      return;
    }
    for (let k = 0; k < rule.numberOfNodes; k++) {
      rule.u[k] = (rule.u[k] + 1.0) / 2.0;
      rule.v[k] = (rule.v[k] + 1.0) / 2.0;
      rule.t[k] = (rule.t[k] + 1.0) / 2.0;
      rule.w[k] /= 8.0;
    }
  }

  private static createTransformedQuadRule(rule: CubatureRule): CubatureRule {
    QuadCubatureRule.transformQuadRuleInPlace(rule);
    return rule;
  }

  private static createTransformedCubeRule(rule: CubatureRule): CubatureRule {
    QuadCubatureRule.transformCubeRuleInPlace(rule);
    return rule;
  }

  public static degree8QuadrilateralRule(): CubatureRule {
    if (QuadCubatureRule.degree8QuadrilateralRuleInstance === null) {
      QuadCubatureRule.degree8QuadrilateralRuleInstance = QuadCubatureRule.createTransformedQuadRule(new CubatureRule(
        "quads degree 8, 16 positions",
        16,
        [
          0.0, 0.0, 0.952509466071562, -0.952509466071562,
          0.532327454074206, -0.532327454074206, 0.684736297951735, -0.684736297951735,
          0.233143240801405, -0.233143240801405, 0.927683319306117, -0.927683319306117,
          0.453120687403749, -0.453120687403749, 0.837503640422812, -0.837503640422812
        ],
        [
          0.659560131960342, -0.949142923043125, 0.765051819557684, 0.765051819557684,
          0.936975981088416, 0.936975981088416, 0.333656717735747, 0.333656717735747,
          -0.079583272377397, -0.079583272377397, -0.272240080612534, -0.272240080612534,
          -0.613735353398028, -0.613735353398028, -0.888477650535971, -0.888477650535971
        ],
        [
          0.0, 0.0, 0.0, 0.0,
          0.0, 0.0, 0.0, 0.0,
          0.0, 0.0, 0.0, 0.0,
          0.0, 0.0, 0.0, 0.0
        ],
        [
          0.450276776305590, 0.166570426777813, 0.098869459933431, 0.098869459933431,
          0.153696747140812, 0.153696747140812, 0.396686976072903, 0.396686976072903,
          0.352014367945695, 0.352014367945695, 0.189589054577798, 0.189589054577798,
          0.375101001147587, 0.375101001147587, 0.125618791640072, 0.125618791640072
        ]
      ));
    }
    return QuadCubatureRule.degree8QuadrilateralRuleInstance;
  }

  public static degree1BoxRule(): CubatureRule {
    if (QuadCubatureRule.degree1BoxRuleInstance === null) {
      QuadCubatureRule.degree1BoxRuleInstance = QuadCubatureRule.createTransformedCubeRule(new CubatureRule(
        "boxes degree 1, 9 positions (the corners + center)",
        9,
        [
          QuadCubatureRule.D_1_9_U, QuadCubatureRule.D_1_9_U, QuadCubatureRule.D_1_9_U, QuadCubatureRule.D_1_9_U,
          -QuadCubatureRule.D_1_9_U, -QuadCubatureRule.D_1_9_U, -QuadCubatureRule.D_1_9_U, -QuadCubatureRule.D_1_9_U, 0.0
        ],
        [
          QuadCubatureRule.D_1_9_U, QuadCubatureRule.D_1_9_U, -QuadCubatureRule.D_1_9_U, -QuadCubatureRule.D_1_9_U,
          QuadCubatureRule.D_1_9_U, QuadCubatureRule.D_1_9_U, -QuadCubatureRule.D_1_9_U, -QuadCubatureRule.D_1_9_U, 0.0
        ],
        [
          QuadCubatureRule.D_1_9_U, -QuadCubatureRule.D_1_9_U, QuadCubatureRule.D_1_9_U, -QuadCubatureRule.D_1_9_U,
          QuadCubatureRule.D_1_9_U, -QuadCubatureRule.D_1_9_U, QuadCubatureRule.D_1_9_U, -QuadCubatureRule.D_1_9_U, 0.0
        ],
        [
          QuadCubatureRule.D_1_9_W, QuadCubatureRule.D_1_9_W, QuadCubatureRule.D_1_9_W, QuadCubatureRule.D_1_9_W,
          QuadCubatureRule.D_1_9_W, QuadCubatureRule.D_1_9_W, QuadCubatureRule.D_1_9_W, QuadCubatureRule.D_1_9_W,
          QuadCubatureRule.D_1_9_W
        ]
      ));
    }
    return QuadCubatureRule.degree1BoxRuleInstance;
  }

  public static setQuadCubatureRules(quadRule: Array<CubatureRule | null>, degree: CubatureDegree): void {
    switch (degree) {
      case CubatureDegree.DEGREE_1:
        quadRule[0] = QuadCubatureRule.crq1;
        break;
      case CubatureDegree.DEGREE_2:
        quadRule[0] = QuadCubatureRule.crq2;
        break;
      case CubatureDegree.DEGREE_3:
        quadRule[0] = QuadCubatureRule.crq3;
        break;
      case CubatureDegree.DEGREE_4:
        quadRule[0] = QuadCubatureRule.crq4;
        break;
      case CubatureDegree.DEGREE_5:
        quadRule[0] = QuadCubatureRule.crq5;
        break;
      case CubatureDegree.DEGREE_6:
        quadRule[0] = QuadCubatureRule.crq6;
        break;
      case CubatureDegree.DEGREE_7:
        quadRule[0] = QuadCubatureRule.crq7;
        break;
      case CubatureDegree.DEGREE_8:
        quadRule[0] = QuadCubatureRule.degree8QuadrilateralRule();
        break;
      case CubatureDegree.DEGREE_9:
        quadRule[0] = QuadCubatureRule.crq9;
        break;
      case CubatureDegree.DEGREE_3_PROD:
        quadRule[0] = QuadCubatureRule.crq3Pg;
        break;
      case CubatureDegree.DEGREE_5_PROD:
        quadRule[0] = QuadCubatureRule.crq5Pg;
        break;
      case CubatureDegree.DEGREE_7_PROD:
        quadRule[0] = QuadCubatureRule.crq7Pg;
        break;
      default:
        VsdkError.fatal(2, "setQuadCubatureRules", "Invalid degree %d", degree);
    }
  }

  private constructor() {
  }
}
