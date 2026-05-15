package vsdk.toolkit.app.options;

public class EnumDesc {
    public int value;
    public String name;
    public int abbrev;

    public EnumDesc() {
        value = 0;
        name = null;
        abbrev = 0;
    }

    public EnumDesc(int value, String name, int abbrev) {
        this.value = value;
        this.name = name;
        this.abbrev = abbrev;
    }
}
