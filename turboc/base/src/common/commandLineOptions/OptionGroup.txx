template<typename TOptionBase>
OptionGroupT<TOptionBase>::OptionGroupT():
    name(NULL),
    options(NULL),
    count(0) {
}

template<typename TOptionBase>
OptionGroupT<TOptionBase>::OptionGroupT(const char *groupName, TOptionBase *groupOptions, int groupCount):
    name(groupName),
    options(groupOptions),
    count(groupCount) {
}
