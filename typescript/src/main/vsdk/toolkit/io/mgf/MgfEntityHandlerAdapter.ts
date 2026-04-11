import { EntityDispatchContext } from "../context/EntityDispatchContext";
import { HandlerRoleContext } from "../context/HandlerRoleContext";
import { ParseContext } from "../context/ParseContext";
import { ParseErrorContext } from "../context/ParseErrorContext";
import { ParseRuntimeContext } from "../context/ParseRuntimeContext";

export type HandlerFunction = (argc: number, argv: string[], context: ParseRuntimeContext) => number;

export class MgfEntityHandlerAdapter extends EntityDispatchContext {
  private readonly handlerType: HandlerRoleContext;
  private readonly handlerFunction: HandlerFunction | null;

  public constructor(handlerType: HandlerRoleContext, handlerFunction: HandlerFunction | null) {
    super();
    this.handlerType = handlerType;
    this.handlerFunction = handlerFunction;
  }

  public override handle(argc: number, argv: string[], context: ParseContext): number {
    if (this.handlerFunction === null) {
      return ParseErrorContext.MGF_OK;
    }
    return this.handlerFunction(argc, argv, context as ParseRuntimeContext);
  }

  public override type(): HandlerRoleContext {
    return this.handlerType;
  }
}
