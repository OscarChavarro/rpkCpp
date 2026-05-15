package vsdk.toolkit.common.commandLineOptions;

public class OptionBase {
    @FunctionalInterface
    public interface ConsumesValueFunction {
        int apply(Object option);
    }

    @FunctionalInterface
    public interface ApplyFunction {
        boolean apply(Object option, Object context, int argc, String[] argv);
    }

    private String name_;
    private int abbreviationLength_;
    private ConsumesValueFunction consumesValue_;
    private ApplyFunction apply_;
    private Object option_;

    public OptionBase() {
        name_ = null;
        abbreviationLength_ = 0;
        consumesValue_ = null;
        apply_ = null;
        option_ = null;
    }

    public OptionBase(
        String name,
        int abbreviationLength,
        ConsumesValueFunction consumesValue,
        ApplyFunction apply,
        Object option)
    {
        name_ = name;
        abbreviationLength_ = abbreviationLength;
        consumesValue_ = consumesValue;
        apply_ = apply;
        option_ = option;
    }

    public boolean isConfigured() {
        return name_ != null && apply_ != null;
    }

    public String getName() {
        return name_;
    }

    public int getAbbreviationLength() {
        return abbreviationLength_;
    }

    public int consumesValue() {
        if (consumesValue_ == null) {
            return 1;
        }
        return consumesValue_.apply(option_);
    }

    public boolean apply(Object context, int argc, String[] argv) {
        if (apply_ == null) {
            return false;
        }
        return apply_.apply(option_, context, argc, argv);
    }
}
