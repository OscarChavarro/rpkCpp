template<typename TOptionBase>
OptionGroupT<TOptionBase>::OptionGroupT():
    name(nullptr),
    options(nullptr),
    count(0) {
}

template<typename TOptionBase>
OptionGroupT<TOptionBase>::OptionGroupT(const char *groupName, TOptionBase *groupOptions, int groupCount):
    name(groupName),
    options(groupOptions),
    count(groupCount) {
}
