import { Error as VsdkError } from "../common/Error";
import { GalerkinElement } from "./GalerkinElement";

export class Interaction {
  private static totalInteractions = 0;
  private static ccInteractions = 0;
  private static csInteractions = 0;
  private static scInteractions = 0;
  private static ssInteractions = 0;

  public receiverElement!: GalerkinElement;
  public sourceElement!: GalerkinElement;
  public K!: number[];
  public deltaK!: number[];
  public numberOfBasisFunctionsOnReceiver!: number;
  public numberOfBasisFunctionsOnSource!: number;
  public numberOfReceiverCubaturePositions!: number;
  public visibility!: number;

  public constructor();
  public constructor(
    inReceiverElement: GalerkinElement,
    inSourceElement: GalerkinElement,
    inK: number[] | null,
    inDeltaK: number[] | null,
    inNumberOfBasisFunctionsOnReceiver: number,
    inNumberOfBasisFunctionsOnSource: number,
    inNumberOfReceiverCubaturePositions: number,
    inVisibility: number,
  );
  public constructor(
    inReceiverElement?: GalerkinElement,
    inSourceElement?: GalerkinElement,
    inK?: number[] | null,
    inDeltaK?: number[] | null,
    inNumberOfBasisFunctionsOnReceiver?: number,
    inNumberOfBasisFunctionsOnSource?: number,
    inNumberOfReceiverCubaturePositions?: number,
    inVisibility?: number,
  ) {
    this.K = [];
    this.deltaK = [0.0];
    this.numberOfBasisFunctionsOnReceiver = 0;
    this.numberOfBasisFunctionsOnSource = 0;
    this.numberOfReceiverCubaturePositions = 0;
    this.visibility = 0;

    if (arguments.length === 0) {
      return;
    }

    this.receiverElement = (inReceiverElement ?? null) as unknown as GalerkinElement;
    this.sourceElement = (inSourceElement ?? null) as unknown as GalerkinElement;
    this.numberOfBasisFunctionsOnReceiver = inNumberOfBasisFunctionsOnReceiver ?? 0;
    this.numberOfBasisFunctionsOnSource = inNumberOfBasisFunctionsOnSource ?? 0;
    this.numberOfReceiverCubaturePositions = inNumberOfReceiverCubaturePositions ?? 0;
    this.visibility = inVisibility ?? 0;

    let n = (this.numberOfBasisFunctionsOnReceiver & 0xff) * (this.numberOfBasisFunctionsOnSource & 0xff);
    if (n <= 0) {
      n = 1;
    }
    this.K = new Array<number>(n).fill(0.0);
    if (inK != null) {
      for (let i = 0; i < Math.min(inK.length, this.K.length); i++) {
        this.K[i] = inK[i];
      }
    }

    if ((this.numberOfReceiverCubaturePositions & 0xff) > 1) {
      VsdkError.fatal(2, "interactionCreate", "Not yet implemented for higher order approximations");
    }

    this.deltaK = new Array<number>(1).fill(0.0);
    if (inDeltaK != null && inDeltaK.length > 0) {
      this.deltaK[0] = inDeltaK[0];
    }

    Interaction.totalInteractions++;
    if (this.receiverElement !== null && this.receiverElement.isCluster()) {
      if (this.sourceElement !== null && this.sourceElement.isCluster()) {
        Interaction.ccInteractions++;
      }
      else {
        Interaction.scInteractions++;
      }
    }
    else {
      if (this.sourceElement !== null && this.sourceElement.isCluster()) {
        Interaction.csInteractions++;
      }
      else {
        Interaction.ssInteractions++;
      }
    }
  }

  public static getNumberOfInteractions(): number {
    return Interaction.totalInteractions;
  }

  public static getNumberOfClusterToClusterInteractions(): number {
    return Interaction.ccInteractions;
  }

  public static getNumberOfClusterToSurfaceInteractions(): number {
    return Interaction.csInteractions;
  }

  public static getNumberOfSurfaceToClusterInteractions(): number {
    return Interaction.scInteractions;
  }

  public static getNumberOfSurfaceToSurfaceInteractions(): number {
    return Interaction.ssInteractions;
  }

  public static interactionDestroy(interaction: Interaction | null): void {
    if (interaction === null) {
      return;
    }

    Interaction.totalInteractions--;
    if (interaction.receiverElement !== null && interaction.receiverElement.isCluster()) {
      if (interaction.sourceElement !== null && interaction.sourceElement.isCluster()) {
        Interaction.ccInteractions--;
      }
      else {
        Interaction.scInteractions--;
      }
    }
    else {
      if (interaction.sourceElement !== null && interaction.sourceElement.isCluster()) {
        Interaction.csInteractions--;
      }
      else {
        Interaction.ssInteractions--;
      }
    }
  }

  public static interactionDuplicate(interaction: Interaction | null): Interaction | null {
    if (interaction === null) {
      return null;
    }
    return new Interaction(
      interaction.receiverElement,
      interaction.sourceElement,
      interaction.K,
      interaction.deltaK,
      interaction.numberOfBasisFunctionsOnReceiver,
      interaction.numberOfBasisFunctionsOnSource,
      interaction.numberOfReceiverCubaturePositions,
      interaction.visibility,
    );
  }
}
