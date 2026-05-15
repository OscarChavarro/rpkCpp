import { OptionBase } from "./OptionBase";
import { OptionGroupT } from "./OptionGroupT";

export class OptionGroup extends OptionGroupT<OptionBase> {
  public constructor();
  public constructor(groupName: string, groupOptions: OptionBase[], groupCount: number);
  public constructor(groupName?: string, groupOptions: OptionBase[] | null = null, groupCount = 0) {
    if (groupName === undefined) {
      super();
      return;
    }
    super(groupName, groupOptions ?? [], groupCount);
  }
}
