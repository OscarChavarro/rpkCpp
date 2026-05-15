package vsdk.toolkit.common.commandLineOptions;

public class DefaultParser<T> {
    @SuppressWarnings("unchecked")
    public static <T> boolean parse(String input, TypedOption.MutableValue<T> out) {
        if (input == null || out == null) {
            return false;
        }

        Object currentValue = out.value;

        if (currentValue instanceof Integer) {
            TypedOption.MutableValue<Integer> converted = new TypedOption.MutableValue<>((Integer)currentValue);
            if (!parseInt(input, converted)) {
                return false;
            }
            out.value = (T)converted.value;
            return true;
        }
        if (currentValue instanceof Long) {
            TypedOption.MutableValue<Long> converted = new TypedOption.MutableValue<>((Long)currentValue);
            if (!parseLong(input, converted)) {
                return false;
            }
            out.value = (T)converted.value;
            return true;
        }
        if (currentValue instanceof Float) {
            TypedOption.MutableValue<Float> converted = new TypedOption.MutableValue<>((Float)currentValue);
            if (!parseFloat(input, converted)) {
                return false;
            }
            out.value = (T)converted.value;
            return true;
        }
        if (currentValue instanceof Boolean) {
            TypedOption.MutableValue<Boolean> converted = new TypedOption.MutableValue<>((Boolean)currentValue);
            if (!parseBoolean(input, converted)) {
                return false;
            }
            out.value = (T)converted.value;
            return true;
        }

        out.value = (T)input;
        return true;
    }

    private static boolean parseInt(String input, TypedOption.MutableValue<Integer> out) {
        if (input == null || out == null) {
            return false;
        }

        try {
            long parsedValue = Long.parseLong(input, 10);
            if (parsedValue < Integer.MIN_VALUE || parsedValue > Integer.MAX_VALUE) {
                return false;
            }
            out.value = (int)parsedValue;
            return true;
        }
        catch (NumberFormatException e) {
            return false;
        }
    }

    private static boolean parseLong(String input, TypedOption.MutableValue<Long> out) {
        if (input == null || out == null) {
            return false;
        }

        try {
            out.value = Long.parseLong(input, 10);
            return true;
        }
        catch (NumberFormatException e) {
            return false;
        }
    }

    private static boolean parseFloat(String input, TypedOption.MutableValue<Float> out) {
        if (input == null || out == null) {
            return false;
        }

        try {
            out.value = Float.parseFloat(input);
            return true;
        }
        catch (NumberFormatException e) {
            return false;
        }
    }

    private static boolean parseBoolean(String input, TypedOption.MutableValue<Boolean> out) {
        if (input == null || out == null) {
            return false;
        }

        if (equalsIgnoreCase(input, "true")
            || equalsIgnoreCase(input, "yes")
            || "1".equals(input)) {
            out.value = true;
            return true;
        }

        if (equalsIgnoreCase(input, "false")
            || equalsIgnoreCase(input, "no")
            || "0".equals(input)) {
            out.value = false;
            return true;
        }

        return false;
    }

    private static char toLowerAscii(char c) {
        if (c >= 'A' && c <= 'Z') {
            return (char)(c - 'A' + 'a');
        }
        return c;
    }

    private static boolean equalsIgnoreCase(String a, String b) {
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
}
