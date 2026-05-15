package vsdk.toolkit.io.context;

public class TokenValidationContext {
    public static boolean isIntDelimited(String text, String delimiters) {
        int cp = skipInt(text);
        if (cp < 0) {
            return false;
        }
        if (cp == text.length()) {
            // C++ reference accepts end-of-string as a valid delimiter ('\0').
            return true;
        }
        return delimiters.indexOf(text.charAt(cp)) >= 0;
    }

    public static boolean isFloatDelimited(String text, String delimiters) {
        int cp = skipFloat(text);
        if (cp < 0) {
            return false;
        }
        if (cp == text.length()) {
            // C++ reference accepts end-of-string as a valid delimiter ('\0').
            return true;
        }
        return delimiters.indexOf(text.charAt(cp)) >= 0;
    }

    public static boolean isFloat(String text) {
        int cp = skipFloat(text);
        return cp >= 0 && cp == text.length();
    }

    public static boolean isInt(String text) {
        int cp = skipInt(text);
        return cp >= 0 && cp == text.length();
    }

    public static boolean isName(String text) {
        int index = 0;
        while (index < text.length() && text.charAt(index) == '_') {
            // skip leading underscores
            index++;
        }
        if (index >= text.length() || !TokenValidationContext.isAsciiCode(text.charAt(index)) || !Character.isLetter(text.charAt(index))) {
            // start with a letter
            return false;
        }
        int tokenIndex = index + 1;
        while (tokenIndex < text.length() && TokenValidationContext.isAsciiCode(text.charAt(tokenIndex))
            && TokenValidationContext.isAsciiGraph(text.charAt(tokenIndex))) {
            // all visible characters
            tokenIndex++;
        }
        return tokenIndex == text.length(); // ending in nul
    }

    private static boolean isAsciiCode(int value) {
        return value >= 0 && value <= 127;
    }

    private static boolean isAsciiGraph(int value) {
        return value >= 33 && value <= 126;
    }

    /**
    Skip integer in string
    */
    private static int skipInt(String text) {
        return skipInt(text, 0);
    }

    private static int skipInt(String text, int start) {
        int index = start;
        while (index < text.length() && Character.isWhitespace(text.charAt(index))) {
            index++;
        }
        if (index < text.length() && (text.charAt(index) == '-' || text.charAt(index) == '+')) {
            index++;
        }
        if (index >= text.length() || !Character.isDigit(text.charAt(index))) {
            return -1;
        }
        do {
            index++;
        } while (index < text.length() && Character.isDigit(text.charAt(index)));
        return index;
    }

    /**
    Skip float in string
    */
    private static int skipFloat(String text) {
        int startIndex = 0;
        while (startIndex < text.length() && Character.isWhitespace(text.charAt(startIndex))) {
            startIndex++;
        }
        if (startIndex < text.length() && (text.charAt(startIndex) == '-' || text.charAt(startIndex) == '+')) {
            startIndex++;
        }
        int currentIndex = startIndex;
        while (currentIndex < text.length() && Character.isDigit(text.charAt(currentIndex))) {
            currentIndex++;
        }
        if (currentIndex < text.length() && text.charAt(currentIndex) == '.') {
            currentIndex++;
            startIndex++;
            while (currentIndex < text.length() && Character.isDigit(text.charAt(currentIndex))) {
                currentIndex++;
            }
        }
        if (currentIndex == startIndex) {
            return -1;
        }
        if (currentIndex < text.length() && (text.charAt(currentIndex) == 'e' || text.charAt(currentIndex) == 'E')) {
            return skipInt(text, currentIndex + 1);
        }
        return currentIndex;
    }
}
