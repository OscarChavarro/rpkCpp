import { TransformStackContext } from "./TransformStackContext";

export class TransformScopeContext {
  public transformContext: TransformStackContext | null;
  public argumentList: Array<string | null> | null;
  public argumentCount: number;
  public iterateArgument: string;

  public constructor() {
    this.transformContext = null;
    this.argumentList = null;
    this.argumentCount = 0;
    this.iterateArgument = "-i";
  }

  public destroy(): void {
    this.clearArguments();
  }

  public clearArguments(): void {
    this.argumentList = null;
    this.argumentCount = 0;
  }

  public argumentCountFor(context: TransformStackContext | null): number {
    return context === null ? 0 : context.xac;
  }

  public argumentStartIndexFor(context: TransformStackContext | null): number {
    return this.argumentCount - this.argumentCountFor(context);
  }

  public argumentVectorFor(context: TransformStackContext | null): Array<string | null> | null {
    if (this.argumentList === null) {
      return null;
    }
    const startIndex = this.argumentStartIndexFor(context);
    if (startIndex < 0 || startIndex >= this.argumentList.length) {
      return null;
    }

    const copy = new Array<string | null>(this.argumentList.length - startIndex);
    for (let i = 0; i < copy.length; i++) {
      copy[i] = this.argumentList[startIndex + i] ?? null;
    }
    return copy;
  }

  public compactTo(context: TransformStackContext | null): boolean {
    const contextArgumentCount = this.argumentCountFor(context);
    let newArgumentList: Array<string | null> | null = null;

    if (contextArgumentCount > 0) {
      newArgumentList = new Array<string | null>(contextArgumentCount + 1).fill(null);
      const sourceStartIndex = this.argumentCount - contextArgumentCount;
      for (let i = 0; i < contextArgumentCount; i++) {
        newArgumentList[i] = (this.argumentList as Array<string | null>)[sourceStartIndex + i] ?? null;
      }
      newArgumentList[contextArgumentCount] = null;
    }

    this.argumentList = newArgumentList;
    this.argumentCount = contextArgumentCount;
    return true;
  }

  public freeTransformContext(context: TransformStackContext | null): void {
    if (context === null) {
      return;
    }
    if (context.ownedArgumentCopies !== null) {
      for (let i = 0; i < context.ownedArgumentCount; i++) {
        context.ownedArgumentCopies[i] = null;
      }
      context.ownedArgumentCopies = null;
    }
    if (context.transformationArray !== null) {
      context.transformationArray = null;
    }
  }
}
