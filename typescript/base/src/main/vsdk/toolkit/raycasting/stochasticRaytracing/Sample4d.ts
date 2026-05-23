/**
4D vector sampling
*/

import { Logger as VsdkLogger } from "../../common/logging/Logger";
import { Faure } from "../../numericalAnalysis/quasiMonteCarlo/Faure";
import { Halton } from "../../numericalAnalysis/quasiMonteCarlo/Halton";
import { Niederreiter31 } from "../../numericalAnalysis/quasiMonteCarlo/Niederreiter31";
import { ScrambledHalton } from "../../numericalAnalysis/quasiMonteCarlo/ScrambledHalton";
import { Sobol } from "../../numericalAnalysis/quasiMonteCarlo/Sobol";
import { Sampler4DSequence } from "./Sampler4DSequence";

export class Sample4d {
  private static seq = Sampler4DSequence.RANDOM;

  private static readonly RANDOM_NAME = "drand48";
  private static readonly HALTON_NAME = "Halton";
  private static readonly SCRAMBLED_HALTON_NAME = "ScramHalton";
  private static readonly SOBOL_NAME = "sobol";
  private static readonly ORIGINAL_FAURE_NAME = "faure";
  private static readonly GENERALIZED_FAURE_NAME = "GFaure";
  private static readonly NIEDERREITER_NAME = "Nied";
  private static readonly UNKNOWN_NAME = "Unknown";

  private static sequenceName(sequence: Sampler4DSequence): string {
    switch (sequence) {
      case Sampler4DSequence.RANDOM:
        return Sample4d.RANDOM_NAME;
      case Sampler4DSequence.HALTON:
        return Sample4d.HALTON_NAME;
      case Sampler4DSequence.SCRAMBLED_HALTON:
        return Sample4d.SCRAMBLED_HALTON_NAME;
      case Sampler4DSequence.SOBOL:
        return Sample4d.SOBOL_NAME;
      case Sampler4DSequence.ORIGINAL_FAURE:
        return Sample4d.ORIGINAL_FAURE_NAME;
      case Sampler4DSequence.GENERALIZED_FAURE:
        return Sample4d.GENERALIZED_FAURE_NAME;
      case Sampler4DSequence.NIEDERREITER:
        return Sample4d.NIEDERREITER_NAME;
      default:
        return Sample4d.UNKNOWN_NAME;
    }
  }

  /**
Also initialises the sequence
*/
  public static setSequence4D(sequence: Sampler4DSequence): void {
    Sample4d.seq = sequence;
    switch (Sample4d.seq) {
      case Sampler4DSequence.SOBOL:
        Sobol.initSobol(4);
        break;
      case Sampler4DSequence.ORIGINAL_FAURE:
        Faure.initOriginalFaureSequence(4);
        break;
      case Sampler4DSequence.GENERALIZED_FAURE:
        Faure.initGeneralizedFaureSequence(4);
        break;
      default:
        break;
    }
  }

  /**
Returns 4D sample with given index from current sequence. When the
current sequence is 'random', the index is not used
*/
  public static sample4D(seed: number): number[] {
    const xi = [0.0, 0.0, 0.0, 0.0];
    let zeta: number[];
    let xx: number[];

    switch (Sample4d.seq) {
      case Sampler4DSequence.RANDOM:
        xi[0] = globalThis.Math.random();
        xi[1] = globalThis.Math.random();
        xi[2] = globalThis.Math.random();
        xi[3] = globalThis.Math.random();
        break;
      case Sampler4DSequence.HALTON:
        xi[0] = Halton.Halton2(seed);
        xi[1] = Halton.Halton3(seed);
        xi[2] = Halton.Halton5(seed);
        xi[3] = Halton.Halton7(seed);
        break;
      case Sampler4DSequence.SCRAMBLED_HALTON:
        xx = ScrambledHalton.scrambledHalton(seed, 4);
        xi[0] = xx[0]!;
        xi[1] = xx[1]!;
        xi[2] = xx[2]!;
        xi[3] = xx[3]!;
        break;
      case Sampler4DSequence.SOBOL:
        xx = Sobol.sobol(seed);
        xi[0] = xx[0]!;
        xi[1] = xx[1]!;
        xi[2] = xx[2]!;
        xi[3] = xx[3]!;
        break;
      case Sampler4DSequence.ORIGINAL_FAURE:
      case Sampler4DSequence.GENERALIZED_FAURE:
        xx = Faure.faure(seed);
        xi[0] = xx[0]!;
        xi[1] = xx[1]!;
        xi[2] = xx[2]!;
        xi[3] = xx[3]!;
        break;
      case Sampler4DSequence.NIEDERREITER:
        zeta = Niederreiter31.niederreiter31(seed);
        xi[0] = zeta[0]! * Niederreiter31.RECIP;
        xi[1] = zeta[1]! * Niederreiter31.RECIP;
        xi[2] = zeta[2]! * Niederreiter31.RECIP;
        xi[3] = zeta[3]! * Niederreiter31.RECIP;
        break;
      default:
        VsdkLogger.fatal(-1, "Sample4d::sample4D", "QMC Sequence %s not yet implemented", Sample4d.sequenceName(Sample4d.seq));
        break;
    }

    return xi;
  }

  /**
The following routines are safe with Sample4D(), which calls only
31-bit sequences (including 31-bit Niederreiter sequence). If
you are looking for such a routine to use directly in conjunction
with the routine Niederreiter::Nied() or Niederreiter::NextNiedInRange(), you should use
the Niederreiter::foldSample() routine in Niederreiter.h instead.
Niederreiter::Nied() and Niederreiter::NextNiedInRange() are 63-bit unless compiled without
'unsigned long long' support
*/
  public static foldSampleU(xi1: number[], xi2: number[]): void {
    Niederreiter31.foldSample31(xi1, xi2);
  }

  public static foldSampleF(xi1: number[], xi2: number[]): void {
    const zeta1 = globalThis.Math.trunc(xi1[0]! * Niederreiter31.RECIP1);
    const zeta2 = globalThis.Math.trunc(xi2[0]! * Niederreiter31.RECIP1);
    Sample4d.foldSampleU([zeta1], [zeta2]);
    xi1[0] = zeta1 * Niederreiter31.RECIP;
    xi2[0] = zeta2 * Niederreiter31.RECIP;
  }
}
