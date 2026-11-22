#ifndef __JAVA_LAMBDA_HOLDER__
#define __JAVA_LAMBDA_HOLDER__

namespace java {

/**
Type erasure behind the functional interfaces of the port (`Runnable`,
`Supplier`, `Consumer`, `BiConsumer`, `Function`, `Predicate`): Java
lambdas and method references become C++ lambdas, stored by these holders.
*/
template <class R, class... A>
class _LambdaHolder {
  public:
    virtual ~_LambdaHolder() {}
    virtual R call(A... arguments) = 0;
    virtual _LambdaHolder *clone() const = 0;
};

template <class F, class R, class... A>
class _LambdaHolderOf : public _LambdaHolder<R, A...> {
  private:
    F function;

  public:
    explicit _LambdaHolderOf(const F &function) : function(function) {}

    R
    call(A... arguments) override
    {
        return function(arguments...);
    }

    _LambdaHolder<R, A...> *
    clone() const override
    {
        return new _LambdaHolderOf<F, R, A...>(function);
    }
};

/**
Value semantics shared by the functional interfaces: copying one copies
its lambda, and a default constructed (or nullptr) one holds none.
*/
template <class R, class... A>
class _Lambda {
  private:
    _LambdaHolder<R, A...> *holder;

  protected:
    R
    invoke(A... arguments) const
    {
        return holder->call(arguments...);
    }

  public:
    _Lambda() : holder(nullptr) {}
    _Lambda(decltype(nullptr)) : holder(nullptr) {}

    _Lambda(const _Lambda &other) :
        holder(other.holder != nullptr ? other.holder->clone() : nullptr)
    {
    }

    template <class F>
    _Lambda(const F &function) :
        holder(new _LambdaHolderOf<F, R, A...>(function))
    {
    }

    _Lambda &
    operator=(const _Lambda &other)
    {
        if ( this != &other ) {
            _LambdaHolder<R, A...> *copy =
                other.holder != nullptr ? other.holder->clone() : nullptr;
            delete holder;
            holder = copy;
        }
        return *this;
    }

    virtual ~_Lambda()
    {
        delete holder;
    }

    /**
    @return true when a lambda is held (Java: the reference is not null)
    */
    explicit operator bool() const
    {
        return holder != nullptr;
    }
};

}

#endif
