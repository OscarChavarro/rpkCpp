import { OptionBase } from "./OptionBase";
import { OptionGroupT } from "./OptionGroupT";
import { TypedOption } from "./TypedOption";

export class OptionParser<TOptionBase extends OptionBase> {
  private constructor() {
  }

  public static parse<TOptionBase extends OptionBase>(
    argc: number[],
    argv: string[],
    registry: TOptionBase[] | null,
    registryCount: number
  ): boolean;
  public static parse<TOptionBase extends OptionBase>(
    argc: number[],
    argv: string[],
    registry: TOptionBase[] | null,
    registryCount: number,
    context: unknown
  ): boolean;
  public static parse<TOptionBase extends OptionBase>(
    argc: number[],
    argv: string[],
    groups: OptionGroupT<TOptionBase>[] | null,
    groupCount: number
  ): boolean;
  public static parse<TOptionBase extends OptionBase>(
    argc: number[],
    argv: string[],
    groups: OptionGroupT<TOptionBase>[] | null,
    groupCount: number,
    context: unknown
  ): boolean;
  public static parse<TOptionBase extends OptionBase>(
    argc: number[],
    argv: string[],
    registryOrGroups: TOptionBase[] | OptionGroupT<TOptionBase>[] | null,
    count: number,
    context: unknown = null
  ): boolean {
    if (
      registryOrGroups === null
      || count <= 0
      || argc === null
      || argc.length === 0
      || argv === null
    ) {
      return false;
    }

    const registryOrGroupList = registryOrGroups as unknown[];
    const firstEntry = registryOrGroupList.length > 0 ? registryOrGroupList[0] : null;
    const firstEntryIsGroup =
      firstEntry !== null
      && typeof firstEntry === "object"
      && "options" in (firstEntry as Record<string, unknown>);

    let groups: OptionGroupT<TOptionBase>[] | null;
    let groupCount: number;

    if (firstEntryIsGroup) {
      groups = registryOrGroups as OptionGroupT<TOptionBase>[];
      groupCount = count;
    }
    else {
      const registry = registryOrGroups as TOptionBase[];
      groups = [new OptionGroupT<TOptionBase>("default", registry, count)];
      groupCount = 1;
    }

    if (groups === null || groupCount <= 0) {
      return false;
    }

    const mutableArgv = argv as Array<string | null>;

    for (let i = 0; i < argc[0]; i++) {
      if (mutableArgv[i] === null) {
        continue;
      }

      let matched = false;
      for (let g = 0; g < groupCount && !matched; g++) {
        const group = groups[g];
        if (group.options === null || group.count <= 0) {
          continue;
        }

        for (let j = 0; j < group.count; j++) {
          const option = group.options[j];

          if (!option.isConfigured()) {
            continue;
          }

          if (
            !TypedOption.matchOption(
              mutableArgv[i],
              option.getName(),
              option.getAbbreviationLength()
            )
          ) {
            continue;
          }

          const consumesValue = option.consumesValue();
          if (consumesValue !== 0) {
            if (i + consumesValue >= argc[0]) {
              return false;
            }

            let missingValue = false;
            for (let k = 1; k <= consumesValue; k++) {
              if (mutableArgv[i + k] === null) {
                missingValue = true;
                break;
              }
            }
            if (missingValue) {
              return false;
            }

            const argsSlice = new Array<string>(consumesValue);
            for (let k = 1; k <= consumesValue; k++) {
              argsSlice[k - 1] = mutableArgv[i + k] as string;
            }

            if (!option.apply(context, consumesValue, argsSlice)) {
              return false;
            }

            mutableArgv[i] = null;
            for (let k = 1; k <= consumesValue; k++) {
              mutableArgv[i + k] = null;
            }

            i += consumesValue;
          }
          else {
            if (!option.apply(context, 0, null)) {
              return false;
            }
            mutableArgv[i] = null;
          }

          matched = true;
          break;
        }
      }
    }

    let writeIndex = 0;
    for (let readIndex = 0; readIndex < argc[0]; readIndex++) {
      if (mutableArgv[readIndex] !== null) {
        mutableArgv[writeIndex++] = mutableArgv[readIndex];
      }
    }
    for (let i = writeIndex; i < argc[0]; i++) {
      mutableArgv[i] = null;
    }

    argc[0] = writeIndex;
    return true;
  }
}
