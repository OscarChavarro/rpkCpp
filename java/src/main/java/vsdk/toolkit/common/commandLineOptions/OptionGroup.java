package vsdk.toolkit.common.commandLineOptions;

public class OptionGroup extends OptionGroupT<OptionBase> {
    public OptionGroup() {
        super();
    }

    public OptionGroup(String groupName, OptionBase[] groupOptions, int groupCount) {
        super(groupName, groupOptions, groupCount);
    }
}
