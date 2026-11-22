import { FaureSequenceLimits } from "./FaureSequenceLimits";

export class Faure {
  private static readonly prime = [2, 3, 5, 5, 7, 7, 11, 11, 11, 11];
  private static ix = Faure.create2DIntArray(FaureSequenceLimits.MAX_DIMENSION, FaureSequenceLimits.MAX_PRIME_DIGITS);
  private static nextFaureSample = new Array<number>(FaureSequenceLimits.MAX_DIMENSION).fill(0.0);
  private static faureSample = new Array<number>(FaureSequenceLimits.MAX_DIMENSION).fill(0.0);
  private static dimension = 0;
  private static primeBase = 0;
  private static nextN = 0;
  private static skip = 0;
  private static nDigits = 0;
  private static generatorMatrix = Faure.create3DIntArray(
    FaureSequenceLimits.MAX_DIMENSION,
    FaureSequenceLimits.MAX_PRIME_DIGITS,
    FaureSequenceLimits.MAX_PRIME_DIGITS
  );

  private static create2DIntArray(rows: number, cols: number): number[][] {
    const out = new Array<number[]>(rows);
    for (let i = 0; i < rows; i++) {
      out[i] = new Array<number>(cols).fill(0);
    }
    return out;
  }

  private static create3DIntArray(a: number, b: number, c: number): number[][][] {
    const out = new Array<number[][]>(a);
    for (let i = 0; i < a; i++) {
      out[i] = Faure.create2DIntArray(b, c);
    }
    return out;
  }

  private static setFaureC(): number {
    for (let j = 0; j < Faure.nDigits; j++) {
      for (let k = j; k < Faure.nDigits; k++) {
        if (j === 0 || j === k) {
          Faure.generatorMatrix[0]![j]![k] = 1;
        }
        else {
          Faure.generatorMatrix[0]![j]![k] =
            (Faure.generatorMatrix[0]![j]![k - 1]! + Faure.generatorMatrix[0]![j - 1]![k - 1]!) % Faure.primeBase;
        }
      }
    }

    for (let i = Faure.dimension - 1; i >= 0; i--) {
      for (let j = 0; j < Faure.nDigits; j++) {
        for (let k = j; k < Faure.nDigits; k++) {
          Faure.generatorMatrix[i]![j]![k] = globalThis.Math.trunc(
            (Faure.generatorMatrix[0]![j]![k]! * globalThis.Math.pow(i, k - j)) % Faure.primeBase
          );
        }
      }
    }

    return 0;
  }

  private static setGFaureC(): number {
    const p = Faure.create2DIntArray(FaureSequenceLimits.MAX_PRIME_DIGITS, FaureSequenceLimits.MAX_PRIME_DIGITS);

    for (let j = 0; j < Faure.nDigits; j++) {
      p[j]![0] = 1;
      p[j]![j] = 1;
    }

    for (let j = 1; j < Faure.nDigits; j++) {
      for (let k = 1; k < j; k++) {
        p[j]![k] = (p[j - 1]![k - 1]! + p[j - 1]![k]!) % Faure.primeBase;
      }
      for (let k = j + 1; k < Faure.nDigits; k++) {
        p[j]![k] = 0;
      }
    }

    for (let i = 0; i < Faure.dimension; i++) {
      for (let m = 0; m < Faure.nDigits; m++) {
        for (let n = 0; n < Faure.nDigits; n++) {
          const qMax = m < n ? m : n;
          Faure.generatorMatrix[i]![m]![n] = 0;
          for (let q = 0; q <= qMax; q++) {
            Faure.generatorMatrix[i]![m]![n] = globalThis.Math.trunc(
              (
                Faure.generatorMatrix[i]![m]![n]!
                + p[m]![q]! * p[n]![q]! * globalThis.Math.pow(i, m + n - 2 * q)
              ) % Faure.primeBase
            );
          }
        }
      }
    }

    return 0;
  }

  private static nextFaure(): number[] {
    let save = Faure.nextN;
    let xx: number;

    let k = 1;
    while ((save % Faure.primeBase) === (Faure.primeBase - 1)) {
      k += 1;
      save = globalThis.Math.trunc(save / Faure.primeBase);
    }

    for (let i = 0; i < Faure.dimension && i < FaureSequenceLimits.MAX_DIMENSION; i++) {
      xx = 0;
      for (let j = Faure.nDigits - 1; j >= 0; j--) {
        if (j < FaureSequenceLimits.MAX_PRIME_DIGITS) {
          Faure.ix[i]![j] = (Faure.ix[i]![j]! + Faure.generatorMatrix[i]![j]![k - 1]!) % Faure.primeBase;
          xx = xx / Faure.primeBase + Faure.ix[i]![j]!;
        }
      }
      Faure.nextFaureSample[i] = xx / Faure.primeBase;
    }

    Faure.nextN += 1;
    return Faure.nextFaureSample;
  }

  public static faure(seed: number): number[] {
    let save: number;
    let xx: number;

    Faure.nextN = seed + Faure.skip + 1;
    for (let i = 0; i < Faure.dimension; i++) {
      xx = 0;
      for (let j = Faure.nDigits - 1; j >= 0; j--) {
        save = Faure.nextN;
        Faure.ix[i]![j] = 0;
        for (let k = 0; k < Faure.nDigits; k++) {
          Faure.ix[i]![j] = (Faure.ix[i]![j]! + Faure.generatorMatrix[i]![j]![k]! * save) % Faure.primeBase;
          save = globalThis.Math.trunc(save / Faure.primeBase);
        }
        xx = xx / Faure.primeBase + Faure.ix[i]![j]!;
      }
      Faure.faureSample[i] = xx / Faure.primeBase;
    }

    return Faure.faureSample;
  }

  public static initOriginalFaureSequence(iDim: number): void {
    Faure.dimension = iDim;
    Faure.nextN = 0;
    Faure.primeBase = Faure.prime[Faure.dimension - 1]!;
    Faure.nDigits = globalThis.Math.trunc(
      globalThis.Math.log(FaureSequenceLimits.MAX_SEED) / globalThis.Math.log(Faure.primeBase) + 1
    );
    Faure.setFaureC();
    for (let i = 0; i < Faure.dimension; i++) {
      for (let j = 0; j < Faure.nDigits; j++) {
        Faure.ix[i]![j] = 0;
      }
    }

    Faure.skip = globalThis.Math.trunc(globalThis.Math.pow(Faure.primeBase, 4.0) - 1);
    for (let i = 1; i <= Faure.skip; i++) {
      Faure.nextFaure();
    }
  }

  public static initGeneralizedFaureSequence(iDim: number): void {
    Faure.dimension = iDim;
    Faure.nextN = 0;
    Faure.primeBase = Faure.prime[Faure.dimension - 1]!;
    Faure.nDigits = globalThis.Math.trunc(
      globalThis.Math.log(FaureSequenceLimits.MAX_SEED) / globalThis.Math.log(Faure.primeBase) + 1
    );
    Faure.setGFaureC();
    for (let i = 0; i < Faure.dimension; i++) {
      for (let j = 0; j < Faure.nDigits; j++) {
        Faure.ix[i]![j] = 0;
      }
    }

    Faure.skip = globalThis.Math.trunc(globalThis.Math.pow(Faure.primeBase, 4.0) - 1);
    for (let i = 1; i <= Faure.skip; i++) {
      Faure.nextFaure();
    }
  }

  private constructor() {
  }
}
