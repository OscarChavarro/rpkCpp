package vsdk.toolkit.common.commandLineOptions;

import java.util.function.Consumer;
import java.util.function.Supplier;

public class TypedOption<T> {
    public interface Reference<T> {
        T get();
        void set(T value);
    }

    public static class ValueRef<T> implements Reference<T> {
        private T value;

        public ValueRef(T initialValue) {
            value = initialValue;
        }

        @Override
        public T get() {
            return value;
        }

        @Override
        public void set(T newValue) {
            value = newValue;
        }
    }

    public static <T> ValueRef<T> valueRef(T initialValue) {
        return new ValueRef<>(initialValue);
    }

    public static <T> Reference<T> reference(Supplier<T> getter, Consumer<T> setter) {
        return new Reference<>() {
            @Override
            public T get() {
                return getter.get();
            }

            @Override
            public void set(T value) {
                setter.accept(value);
            }
        };
    }

    public static class MutableValue<T> {
        public T value;

        public MutableValue(T value) {
            this.value = value;
        }
    }

    @FunctionalInterface
    public interface OnSet<T> {
        void apply(MutableValue<T> value);
    }

    @FunctionalInterface
    public interface ParseArgs<T> {
        boolean apply(int argc, String[] argv, MutableValue<T> value);
    }

    @FunctionalInterface
    public interface Validate<T> {
        boolean apply(MutableValue<T> value);
    }

    @FunctionalInterface
    public interface Transform<T> {
        void apply(MutableValue<T> value);
    }

    private String name_;
    private Reference<T> target_;
    private int consumesValue_;
    private OnSet<T> onSet_;
    private ParseArgs<T> parseArgs_;
    private Validate<T> validate_;
    private Transform<T> transform_;

    public TypedOption() {
        name_ = null;
        target_ = null;
        consumesValue_ = 1;
        onSet_ = null;
        parseArgs_ = null;
        validate_ = null;
        transform_ = null;
    }

    public TypedOption(String name, Reference<T> target, int consumesValue, OnSet<T> onSet, ParseArgs<T> parseArgs) {
        name_ = name;
        target_ = target;
        consumesValue_ = consumesValue;
        onSet_ = onSet;
        parseArgs_ = parseArgs;
        validate_ = null;
        transform_ = null;
    }

    public TypedOption(String name, Reference<T> target, int consumesValue, OnSet<T> onSet, ParseArgs<T> parseArgs, Validate<T> validate, Transform<T> transform) {
        name_ = name;
        target_ = target;
        consumesValue_ = consumesValue;
        onSet_ = onSet;
        parseArgs_ = parseArgs;
        validate_ = validate;
        transform_ = transform;
    }

    public String getName() {
        return name_;
    }

    public int getConsumesValue() {
        return consumesValue_;
    }

    public boolean apply(int argc, String[] argv) {
        Reference<T> target = target_;
        if (target == null) {
            return false;
        }

        MutableValue<T> value = new MutableValue<>(target.get());

        if (consumesValue_ == 0) {
            if (parseArgs_ != null) {
                if (!parseArgs_.apply(0, null, value)) {
                    return false;
                }
            }
            if (validate_ != null && !validate_.apply(value)) {
                return false;
            }
            if (transform_ != null) {
                transform_.apply(value);
            }
            target.set(value.value);
            if (onSet_ != null) {
                MutableValue<T> targetValue = new MutableValue<>(target.get());
                onSet_.apply(targetValue);
                target.set(targetValue.value);
            }
            return true;
        }

        if (argc < consumesValue_) {
            return false;
        }

        boolean parsed;
        if (parseArgs_ != null) {
            parsed = parseArgs_.apply(consumesValue_, argv, value);
        }
        else if (consumesValue_ == 1) {
            parsed = DefaultParser.parse(argv[0], value);
        }
        else {
            parsed = false;
        }

        if (!parsed) {
            return false;
        }

        if (validate_ != null && !validate_.apply(value)) {
            return false;
        }
        if (transform_ != null) {
            transform_.apply(value);
        }
        target.set(value.value);
        if (onSet_ != null) {
            MutableValue<T> targetValue = new MutableValue<>(target.get());
            onSet_.apply(targetValue);
            target.set(targetValue.value);
        }

        return true;
    }

    public static boolean matchOption(String input, String name, int abbrLen) {
        if (input == null || name == null) {
            return false;
        }
        if (input.equals(name)) {
            return true;
        }
        if (abbrLen <= 0) {
            return false;
        }

        int inputLength = input.length();
        if (inputLength > abbrLen) {
            return false;
        }

        return name.regionMatches(0, input, 0, inputLength);
    }

    public static <T> OptionBase registerOption(TypedOption<T> optionInstance, int abbr) {
        return new OptionBase(optionInstance, abbr);
    }

    public static <T> OptionBase REGISTER_OPTION(TypedOption<T> optionInstance, int abbr) {
        return registerOption(optionInstance, abbr);
    }
}
