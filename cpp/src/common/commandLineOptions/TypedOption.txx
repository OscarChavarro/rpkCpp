template<typename T>
TypedOption<T>::TypedOption():
    name_(nullptr),
    target_(nullptr),
    consumesValue_(1),
    onSet_(nullptr),
    parseArgs_(nullptr) {
}

template<typename T>
TypedOption<T>::TypedOption(const char *name, T *target, int consumesValue, void (*onSet)(T &), bool (*parseArgs)(int, char **, T &)):
    name_(name),
    target_(target),
    consumesValue_(consumesValue),
    onSet_(onSet),
    parseArgs_(parseArgs) {
}

template<typename T>
const char *TypedOption<T>::getName() const {
    return name_;
}

template<typename T>
int TypedOption<T>::getConsumesValue() const {
    return consumesValue_;
}

template<typename T>
bool TypedOption<T>::apply(int argc, char **argv) {
    if ( target_ == nullptr ) {
        return false;
    }
    if ( consumesValue_ == 0 ) {
        if ( parseArgs_ != nullptr ) {
            if ( !parseArgs_(0, nullptr, *target_) ) {
                return false;
            }
        }
        if ( onSet_ != nullptr ) {
            onSet_(*target_);
        }
        return true;
    }
    if ( argc < consumesValue_ ) {
        return false;
    }
    T value = *target_;
    bool parsed = false;
    if ( parseArgs_ != nullptr ) {
        parsed = parseArgs_(consumesValue_, argv, value);
    } else if ( consumesValue_ == 1 ) {
        parsed = DefaultParser<T>::parse(argv[0], value);
    }
    if ( !parsed ) {
        return false;
    }
    *target_ = value;
    if ( onSet_ != nullptr ) {
        onSet_(*target_);
    }
    return true;
}

inline OptionBase::OptionBase():
    name_(nullptr),
    abbreviationLength_(0),
    consumesValue_(nullptr),
    apply_(nullptr),
    option_(nullptr) {
}

inline OptionBase::OptionBase(
        const char *name,
        int abbreviationLength,
        int (*consumesValue)(void *),
        bool (*apply)(void *, int, char **),
        void *option):
    name_(name),
    abbreviationLength_(abbreviationLength),
    consumesValue_(consumesValue),
    apply_(apply),
    option_(option) {
}

inline bool OptionBase::isConfigured() const {
    return name_ != nullptr && apply_ != nullptr;
}

inline const char *OptionBase::getName() const {
    return name_;
}

inline int OptionBase::getAbbreviationLength() const {
    return abbreviationLength_;
}

inline int OptionBase::consumesValue() const {
    if ( consumesValue_ == nullptr ) {
        return 1;
    }
    return consumesValue_(option_);
}

inline bool OptionBase::apply(int argc, char **argv) const {
    if ( apply_ == nullptr ) {
        return false;
    }
    return apply_(option_, argc, argv);
}

template<typename T>
bool applyOption(TypedOption<T> &opt, int argc, char **argv) {
    return opt.apply(argc, argv);
}

template<typename T>
bool applyAdapter(void *opt, int argc, char **argv) {
    if ( opt == nullptr ) {
        return false;
    }
    return applyOption(*(TypedOption<T> *)opt, argc, argv);
}

template<typename T>
int consumesValueAdapter(void *opt) {
    if ( opt == nullptr ) {
        return 1;
    }
    return ((TypedOption<T> *)opt)->getConsumesValue();
}
