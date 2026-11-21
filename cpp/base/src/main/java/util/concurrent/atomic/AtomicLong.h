#ifndef __ATOMIC_LONG__
#define __ATOMIC_LONG__

namespace java {

/**
Emulation of `java.util.concurrent.atomic.AtomicLong`, over the atomic
builtins of the compiler (sequentially consistent, as Java volatile).
*/
class AtomicLong {
private:
    alignas(8) long long value_;

public:
    AtomicLong() : value_(0) {}
    explicit AtomicLong(long long initialValue) : value_(initialValue) {}

    AtomicLong(const AtomicLong&) = delete;
    AtomicLong& operator=(const AtomicLong&) = delete;

    long long get() const { return __atomic_load_n(&value_, __ATOMIC_SEQ_CST); }
    void set(long long value) { __atomic_store_n(&value_, value, __ATOMIC_SEQ_CST); }
    long long incrementAndGet() { return __atomic_add_fetch(&value_, 1, __ATOMIC_SEQ_CST); }
    long long getAndIncrement() { return __atomic_fetch_add(&value_, 1, __ATOMIC_SEQ_CST); }
    long long decrementAndGet() { return __atomic_sub_fetch(&value_, 1, __ATOMIC_SEQ_CST); }
    long long addAndGet(long long delta) { return __atomic_add_fetch(&value_, delta, __ATOMIC_SEQ_CST); }
    long long getAndAdd(long long delta) { return __atomic_fetch_add(&value_, delta, __ATOMIC_SEQ_CST); }
    long long getAndSet(long long value) { return __atomic_exchange_n(&value_, value, __ATOMIC_SEQ_CST); }

    bool compareAndSet(long long expectedValue, long long newValue)
    {
        return __atomic_compare_exchange_n(&value_, &expectedValue, newValue, false,
                                           __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    }
};

}

#endif
