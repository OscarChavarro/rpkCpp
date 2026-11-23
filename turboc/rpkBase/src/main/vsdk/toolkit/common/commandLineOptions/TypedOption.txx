template<typename T>
TypedOption<T>::TypedOption():
    name_(NULL),
    target_(NULL),
    consumesValue_(1),
    onSet_(NULL),
    parseArgs_(NULL),
    validate_(NULL),
    transform_(NULL) {
}

template<typename T>
TypedOption<T>::TypedOption(const char *name, T *target, int consumesValue, void (*onSet)(T &), bool (*parseArgs)(int, char **, T &)):
    name_(name),
    target_(target),
    consumesValue_(consumesValue),
    onSet_(onSet),
    parseArgs_(parseArgs),
    validate_(NULL),
    transform_(NULL) {
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
    if ( target == NULL ) {
        return false;
    }
    T value = *target;
    if ( consumesValue_ == 0 ) {
        if ( parseArgs_ != NULL ) {
            if ( !parseArgs_(0, NULL, value) ) {
                return false;
            }
        }
        if ( validate_ != NULL && !validate_(value) ) {
            return false;
        }
        if ( transform_ != NULL ) {
            transform_(value);
        }
        *target = value;
        if ( onSet_ != NULL ) {
            onSet_(*target);
        }
        return true;
    }
    if ( argc < consumesValue_ ) {
        return false;
    }
    bool parsed = false;
    if ( parseArgs_ != NULL ) {
        parsed = parseArgs_(consumesValue_, argv, value);
    } else if ( consumesValue_ == 1 ) {
        parsed = DefaultParser<T>::parse(argv[0], value);
    }
    if ( !parsed ) {
        return false;
    }
    if ( validate_ != NULL && !validate_(value) ) {
        return false;
    }
    if ( transform_ != NULL ) {
        transform_(value);
    }
    *target = value;
    if ( onSet_ != NULL ) {
        onSet_(*target);
    }
    return true;
}

inline OptionBase::OptionBase():
    option_(NULL), abbreviationLength_(0) {
}

inline OptionBase::OptionBase(OptionValue *option, int abbreviationLength):
    option_(option), abbreviationLength_(abbreviationLength) {
}

inline bool OptionBase::isConfigured() const {
    return option_ != NULL && option_->getName() != NULL;
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
    return option_ != NULL && option_->apply(argc, argv);
}
