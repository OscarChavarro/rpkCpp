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
    const argCount = argc[0] ?? 0;

    for (let i = 0; i < argCount; i++) {
      if (mutableArgv[i] === null) {
        continue;
      }

      let matched = false;
      for (let g = 0; g < groupCount && !matched; g++) {
        const group = groups[g];
        if (group === undefined) {
          continue;
        }
        if (group.options === null || group.count <= 0) {
          continue;
        }

        for (let j = 0; j < group.count; j++) {
          const option = group.options[j];
          if (option === undefined) {
            continue;
          }

          if (!option.isConfigured()) {
            continue;
          }

          if (
            !TypedOption.matchOption(
              mutableArgv[i] ?? null,
              option.getName(),
              option.getAbbreviationLength()
            )
          ) {
            continue;
          }

          const consumesValue = option.consumesValue();
          if (consumesValue !== 0) {
            if (i + consumesValue >= argCount) {
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
    for (let readIndex = 0; readIndex < argCount; readIndex++) {
      const arg = mutableArgv[readIndex];
      if (arg !== null && arg !== undefined) {
        mutableArgv[writeIndex++] = arg;
      }
    }
    for (let i = writeIndex; i < argCount; i++) {
      mutableArgv[i] = null;
    }

    argc[0] = writeIndex;
    return true;
  }
}
