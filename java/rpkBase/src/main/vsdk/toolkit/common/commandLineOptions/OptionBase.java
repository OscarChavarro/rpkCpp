package vsdk.toolkit.common.commandLineOptions;

public class OptionBase {
    private final TypedOption<?> option;
    private final int abbreviationLength;

    public OptionBase() { this(null, 0); }
    public OptionBase(TypedOption<?> option, int abbreviationLength) {
        this.option = option;
        this.abbreviationLength = abbreviationLength;
    }
    public boolean isConfigured() { return option != null && option.getName() != null; }
    public String getName() { return option.getName(); }
    public int getAbbreviationLength() { return abbreviationLength; }
    public int consumesValue() { return option.getConsumesValue(); }
    public boolean apply(int argc, String[] argv) { return option != null && option.apply(argc, argv); }
}
