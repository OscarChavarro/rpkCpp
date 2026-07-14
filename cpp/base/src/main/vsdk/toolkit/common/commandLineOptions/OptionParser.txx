template<typename TOptionBase>
bool OptionParser<TOptionBase>::parse(int *argc, char **argv, TOptionBase *registry, int registryCount, void *context) {
    OptionGroupT<TOptionBase> singleGroup("default", registry, registryCount);
    return parse(argc, argv, &singleGroup, 1, context);
}

template<typename TOptionBase>
bool OptionParser<TOptionBase>::parse(int *argc, char **argv, OptionGroupT<TOptionBase> *groups, int groupCount, void *context) {
    if ( groups == nullptr || groupCount <= 0 || argc == nullptr || argv == nullptr ) {
        return false;
    }

    for ( int i = 0; i < *argc; i++ ) {
        if ( argv[i] == nullptr ) {
            continue;
        }
        bool matched = false;
        for ( int g = 0; g < groupCount && !matched; g++ ) {
            if ( groups[g].options == nullptr || groups[g].count <= 0 ) {
                continue;
            }
            for ( int j = 0; j < groups[g].count; j++ ) {
                if ( !groups[g].options[j].isConfigured() ) {
                    continue;
                }
                if ( !matchOption(argv[i], groups[g].options[j].getName(), groups[g].options[j].getAbbreviationLength()) ) {
                    continue;
                }
                int consumesValue = groups[g].options[j].consumesValue();
                if ( consumesValue != 0 ) {
                    if ( i + consumesValue >= *argc ) {
                        return false;
                    }
                    bool missingValue = false;
                    for ( int k = 1; k <= consumesValue; k++ ) {
                        if ( argv[i + k] == nullptr ) {
                            missingValue = true;
                            break;
                        }
                    }
                    if ( missingValue ) {
                        return false;
                    }
                    if ( !groups[g].options[j].apply(context, consumesValue, argv + i + 1) ) {
                        return false;
                    }
                    argv[i] = nullptr;
                    for ( int k = 1; k <= consumesValue; k++ ) {
                        argv[i + k] = nullptr;
                    }
                    i += consumesValue;
                } else {
                    if ( !groups[g].options[j].apply(context, 0, nullptr) ) {
                        return false;
                    }
                    argv[i] = nullptr;
                }
                matched = true;
                break;
            }
        }
    }

    int writeIndex = 0;
    for ( int readIndex = 0; readIndex < *argc; readIndex++ ) {
        if ( argv[readIndex] != nullptr ) {
            argv[writeIndex++] = argv[readIndex];
        }
    }
    for ( int i = writeIndex; i < *argc; i++ ) {
        argv[i] = nullptr;
    }
    *argc = writeIndex;
    return true;
}
