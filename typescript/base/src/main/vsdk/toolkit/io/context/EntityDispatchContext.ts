import { HandlerRoleContext } from "./HandlerRoleContext";
import { ParseContext } from "./ParseContext";

export abstract class EntityDispatchContext {
  public abstract handle(argc: number, argv: string[], context: ParseContext): number;

  public abstract type(): HandlerRoleContext;
}
