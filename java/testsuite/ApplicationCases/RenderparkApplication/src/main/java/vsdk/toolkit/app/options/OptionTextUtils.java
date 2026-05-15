package vsdk.toolkit.app.options;

final class OptionTextUtils {
    private OptionTextUtils() {
    }

    private static char toLowerAscii(char c) {
        if (c >= 'A' && c <= 'Z') {
            return (char)(c - 'A' + 'a');
        }
        return c;
    }

    static boolean equalsIgnoreCase(String a, String b) {
        if (a == null || b == null) {
            return false;
        }
        if (a.length() != b.length()) {
            return false;
        }
        for (int i = 0; i < a.length(); i++) {
            if (toLowerAscii(a.charAt(i)) != toLowerAscii(b.charAt(i))) {
                return false;
            }
        }
        return true;
    }

    static boolean equalsIgnoreCasePrefix(String input, String name, int prefixLength) {
        if (input == null || name == null || prefixLength <= 0) {
            return false;
        }
        if (input.length() < prefixLength || name.length() < prefixLength) {
            return false;
        }
        for (int i = 0; i < prefixLength; i++) {
            if (toLowerAscii(input.charAt(i)) != toLowerAscii(name.charAt(i))) {
                return false;
            }
        }
        return true;
    }

    static boolean parseBoolInt(String text, TypedIntValue out) {
        if (text == null || out == null) {
            return false;
        }
        if (equalsIgnoreCase(text, "true") || equalsIgnoreCase(text, "yes") || "1".equals(text)) {
            out.value = 1;
            return true;
        }
        if (equalsIgnoreCase(text, "false") || equalsIgnoreCase(text, "no") || "0".equals(text)) {
            out.value = 0;
            return true;
        }
        return false;
    }

    static final class TypedIntValue {
        int value;

        TypedIntValue(int value) {
            this.value = value;
        }
    }
}
