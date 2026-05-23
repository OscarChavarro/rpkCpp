export class DiscreteSampling {
  private constructor() {
  }

  public static sample(
    probabilities: number[],
    total: number,
    x1: number[],
    probabilityDensityFunction: number[]
  ): number {
    let i = 0;
    let sample = x1[0]! * total;
    let sum = probabilities[0]!;

    while (sample > sum) {
      i++;
      sum += probabilities[i]!;
    }

    // Rescale x_1
    const left = sum - probabilities[i]!;

    x1[0] = ((sample - left) / (sum - left));
    probabilityDensityFunction[0] = probabilities[i]! / total;
    return i;
  }
}
