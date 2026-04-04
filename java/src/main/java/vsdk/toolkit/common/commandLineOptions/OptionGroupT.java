package vsdk.toolkit.common.commandLineOptions;

public class OptionGroupT<TOptionBase extends OptionBase> {
    public String name;
    public TOptionBase[] options;
    public int count;

    public OptionGroupT() {
        name = null;
        options = null;
        count = 0;
    }

    public OptionGroupT(String groupName, TOptionBase[] groupOptions, int groupCount) {
        name = groupName;
        options = groupOptions;
        count = groupCount;
    }
}
