import { LookUpBehaviors } from "../../common/dataStructures/LookUpBehaviors";
import { LookUpTable } from "../../common/dataStructures/LookUpTable";
import { EntityDispatchContext } from "./EntityDispatchContext";
import { EntityTypeContext } from "./EntityTypeContext";
import { HandlerRoleContext } from "./HandlerRoleContext";
import { ParseErrorContext } from "./ParseErrorContext";
import { ReaderContext } from "./ReaderContext";

export class ReaderDispatchContext {
  private static readonly TOTAL_MGF_HANDLER_TYPES = HandlerRoleContext.HANDLE_OBJECT + 1;

  public entityNames: string[];
  public errorCodeMessages: string[];
  public entityLookUpTable: LookUpTable<string>;
  public nextFileContextId: number;
  public readerContext: ReaderContext | null;
  public handleCallbacks: Array<EntityDispatchContext | null>;
  public supportCallbacks: Array<EntityDispatchContext | null>;
  public handlerByType: Array<EntityDispatchContext | null>;

  public static handlerTypeCount(): number {
    return ReaderDispatchContext.TOTAL_MGF_HANDLER_TYPES;
  }

  public constructor() {
    this.entityNames = new Array<string>(EntityTypeContext.TOTAL_NUMBER_OF_ENTITIES);
    this.errorCodeMessages = new Array<string>(ParseErrorContext.MGF_NUMBER_OF_ERRORS);
    this.entityLookUpTable = new LookUpTable<string>(LookUpBehaviors.NON_OWNING);
    this.nextFileContextId = 0;
    this.readerContext = null;
    this.handleCallbacks = new Array<EntityDispatchContext | null>(EntityTypeContext.TOTAL_NUMBER_OF_ENTITIES).fill(null);
    this.supportCallbacks = new Array<EntityDispatchContext | null>(EntityTypeContext.TOTAL_NUMBER_OF_ENTITIES).fill(null);
    this.handlerByType = new Array<EntityDispatchContext | null>(ReaderDispatchContext.TOTAL_MGF_HANDLER_TYPES).fill(null);

    this.entityNames[0] = "#";
    this.entityNames[1] = "c";
    this.entityNames[2] = "cct";
    this.entityNames[3] = "cone";
    this.entityNames[4] = "cmix";
    this.entityNames[5] = "cspec";
    this.entityNames[6] = "cxy";
    this.entityNames[7] = "cyl";
    this.entityNames[8] = "ed";
    this.entityNames[9] = "f";
    this.entityNames[10] = "i";
    this.entityNames[11] = "ies";
    this.entityNames[12] = "ir";
    this.entityNames[13] = "m";
    this.entityNames[14] = "n";
    this.entityNames[15] = "o";
    this.entityNames[16] = "p";
    this.entityNames[17] = "prism";
    this.entityNames[18] = "rd";
    this.entityNames[19] = "ring";
    this.entityNames[20] = "rs";
    this.entityNames[21] = "sides";
    this.entityNames[22] = "sph";
    this.entityNames[23] = "td";
    this.entityNames[24] = "torus";
    this.entityNames[25] = "ts";
    this.entityNames[26] = "v";
    this.entityNames[27] = "xf";
    this.entityNames[28] = "fh";

    this.errorCodeMessages[0] = "No error";
    this.errorCodeMessages[1] = "Unknown entity";
    this.errorCodeMessages[2] = "Wrong number of arguments";
    this.errorCodeMessages[3] = "Wrong argument type";
    this.errorCodeMessages[4] = "Illegal argument value";
    this.errorCodeMessages[5] = "Undefined reference";
    this.errorCodeMessages[6] = "Cannot open input file";
    this.errorCodeMessages[7] = "Error in included file";
    this.errorCodeMessages[8] = "Out of memory";
    this.errorCodeMessages[9] = "Seek failure";
    this.errorCodeMessages[10] = "Illegal material specification";
    this.errorCodeMessages[11] = "Input line too long";
    this.errorCodeMessages[12] = "Unmatched context close";
  }

  public destroy(): void {
    for (let i = 0; i < ReaderDispatchContext.handlerTypeCount(); i++) {
      if (this.handlerByType[i] !== null) {
        this.handlerByType[i] = null;
      }
    }
  }
}
