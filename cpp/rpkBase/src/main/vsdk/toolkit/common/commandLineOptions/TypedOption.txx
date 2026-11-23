template<typename T>
TypedOption<T>::TypedOption():
    name_(nullptr),
    target_(nullptr),
    consumesValue_(1),
    onSet_(nullptr),
    parseArgs_(nullptr),
    validate_(nullptr),
    transform_(nullptr) {
}

template<typename T>
TypedOption<T>::TypedOption(const char *name, T *target, int consumesValue, void (*onSet)(T &), bool (*parseArgs)(int, char **, T &)):
    name_(name),
    target_(target),
    consumesValue_(consumesValue),
    onSet_(onSet),
    parseArgs_(parseArgs),
    validate_(nullptr),
    transform_(nullptr) {
}

template<typename T>
TypedOption<T>::TypedOption(const char *name, T *target, int consumesValue, void (*onSet)(T &), bool (*parseArgs)(int, char **, T &), bool (*validate)(T &), void (*transform)(T &)):
    name_(name),
    target_(target),
    consumesValue_(consumesValue),
    onSet_(onSet),
    parseArgs_(parseArgs),
    validate_(validate),
    transform_(transform) {
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
    T *target = target_;
    if ( target == nullptr ) {
        return false;
    }
    T value = *target;
    if ( consumesValue_ == 0 ) {
        if ( parseArgs_ != nullptr ) {
            if ( !parseArgs_(0, nullptr, value) ) {
                return false;
            }
        }
        if ( validate_ != nullptr && !validate_(value) ) {
            return false;
        }
        if ( transform_ != nullptr ) {
            transform_(value);
        }
        *target = value;
        if ( onSet_ != nullptr ) {
            onSet_(*target);
        }
        return true;
    }
    if ( argc < consumesValue_ ) {
        return false;
    }
    bool parsed = false;
    if ( parseArgs_ != nullptr ) {
        parsed = parseArgs_(consumesValue_, argv, value);
    } else if ( consumesValue_ == 1 ) {
        parsed = DefaultParser<T>::parse(argv[0], value);
    }
    if ( !parsed ) {
        return false;
    }
    if ( validate_ != nullptr && !validate_(value) ) {
        return false;
    }
    if ( transform_ != nullptr ) {
        transform_(value);
    }
    *target = value;
    if ( onSet_ != nullptr ) {
        onSet_(*target);
    }
    return true;
}

inline OptionBase::OptionBase():
    option_(nullptr), abbreviationLength_(0) {
}

inline OptionBase::OptionBase(OptionValue *option, int abbreviationLength):
    option_(option), abbreviationLength_(abbreviationLength) {
}

inline bool OptionBase::isConfigured() const {
    return option_ != nullptr && option_->getName() != nullptr;
}

inline const char *OptionBase::getName() const {
    return option_->getName();
}

inline int OptionBase::getAbbreviationLength() const {
    return abbreviationLength_;
}

inline int OptionBase::consumesValue() const {
    return option_->getConsumesValue();
}

inline bool OptionBase::apply(int argc, char **argv) const {
    return option_ != nullptr && option_->apply(argc, argv);
}
