#ifndef __VALUE_PARSER__
#define __VALUE_PARSER__

template<typename T>
struct ValueParser {
    static bool parse(const char *input, T &out);
};

template<>
struct ValueParser<int> {
    static bool parse(const char *input, int &out);
};

template<>
struct ValueParser<float> {
    static bool parse(const char *input, float &out);
};

template<>
struct ValueParser<bool> {
    static bool parse(const char *input, bool &out);
};

template<>
struct ValueParser<char *> {
    static bool parse(const char *input, char *&out);
};

#endif
