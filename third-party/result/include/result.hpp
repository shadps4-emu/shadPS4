/* 
   Mathieu Stefani, 03 mai 2016
   
   This header provides a Result type that can be used to replace exceptions in code
   that has to handle error.

   Result<T, E> can be used to return and propagate an error to the caller. Result<T, E> is an algebraic
   data type that can either Ok(T) to represent success or Err(E) to represent an error.
*/

#pragma once

#include <iostream>
#include <functional>
#include <type_traits>

namespace types {
    template<typename T>
    struct Ok {
        Ok(const T& val) : val(val) { }
        Ok(T&& val) : val(std::move(val)) { }

        T val;
    };

    template<>
    struct Ok<void> { };

    template<typename E>
    struct Err {
        Err(const E& val) : val(val) { }
        Err(E&& val) : val(std::move(val)) { }

        E val;
    };
}

template<typename T, typename CleanT = typename std::decay<T>::type>
types::Ok<CleanT> Ok(T&& val) {
    return types::Ok<CleanT>(std::forward<T>(val));
}

inline types::Ok<void> Ok() {
    return types::Ok<void>();
}

template<typename E, typename CleanE = typename std::decay<E>::type>
types::Err<CleanE> Err(E&& val) {
    return types::Err<CleanE>(std::forward<E>(val));
}

namespace Rust {
template<typename T, typename E> struct Result;
}

namespace details {

template<typename ...> struct void_t { typedef void type; };

namespace impl {
    template<typename Func> struct result_of;

    template<typename Ret, typename Cls, typename... Args>
    struct result_of<Ret (Cls::*)(Args...)> : public result_of<Ret (Args...)> { };

    template<typename Ret, typename... Args>
    struct result_of<Ret (Args...)> {
        typedef Ret type;
    };
}

template<typename Func>
struct result_of : public impl::result_of<decltype(&Func::operator())> { };

template<typename Ret, typename Cls, typename... Args>
struct result_of<Ret (Cls::*) (Args...) const> {
    typedef Ret type;
};

template<typename Ret, typename... Args>
struct result_of<Ret (*)(Args...)> {
    typedef Ret type;
};

template<typename R>
struct ResultOkType { typedef typename std::decay<R>::type type; };

template<typename T, typename E>
struct ResultOkType<Rust::Result<T, E>> {
    typedef T type;
};

template<typename R>
struct ResultErrType { typedef R type; };

template<typename T, typename E>
struct ResultErrType<Rust::Result<T, E>> {
    typedef typename std::remove_reference<E>::type type;
};

template<typename R> struct IsResult : public std::false_type { };
template<typename T, typename E>
struct IsResult<Rust::Result<T, E>> : public std::true_type { };

namespace ok {

namespace impl {

template<typename T> struct Map;

template<typename Ret, typename Cls, typename Arg>
struct Map<Ret (Cls::*)(Arg) const> : public Map<Ret (Arg)> { };

template<typename Ret, typename Cls, typename Arg>
struct Map<Ret (Cls::*)(Arg)> : public Map<Ret (Arg)> { };

// General implementation
template<typename Ret, typename Arg>
struct Map<Ret (Arg)> {

    static_assert(!IsResult<Ret>::value,
            "Can not map a callback returning a Result, use andThen instead");

    template<typename T, typename E, typename Func>
    static Rust::Result<Ret, E> map(const Rust::Result<T, E>& result, Func func) {

        static_assert(
                std::is_same<T, Arg>::value ||
                std::is_convertible<T, Arg>::value,
                "Incompatible types detected");

        if (result.isOk()) {
            auto res = func(result.storage().template get<T>());
            return types::Ok<Ret>(std::move(res));
        }

        return types::Err<E>(result.storage().template get<E>());
    }
};

// Specialization for callback returning void
template<typename Arg>
struct Map<void (Arg)> {

    template<typename T, typename E, typename Func>
    static Rust::Result<void, E> map(const Rust::Result<T, E>& result, Func func) {

        if (result.isOk()) {
            func(result.storage().template get<T>());
            return types::Ok<void>();
        }

        return types::Err<E>(result.storage().template get<E>());
    }
};

// Specialization for a void Result
template<typename Ret>
struct Map<Ret (void)> {

    template<typename T, typename E, typename Func>
    static Rust::Result<Ret, E> map(const Rust::Result<T, E>& result, Func func) {
        static_assert(std::is_same<T, void>::value,
                "Can not map a void callback on a non-void Result");

        if (result.isOk()) {
            auto ret = func();
            return types::Ok<Ret>(std::move(ret));
        }

        return types::Err<E>(result.storage().template get<E>());
    }
};

// Specialization for callback returning void on a void Result
template<>
struct Map<void (void)> {

    template<typename T, typename E, typename Func>
    static Rust::Result<void, E> map(const Rust::Result<T, E>& result, Func func) {
        static_assert(std::is_same<T, void>::value,
                "Can not map a void callback on a non-void Result");

        if (result.isOk()) {
            func();
            return types::Ok<void>();
        }

        return types::Err<E>(result.storage().template get<E>());
    }
};

// General specialization for a callback returning a Result
template<typename U, typename E, typename Arg>
struct Map<Rust::Result<U, E> (Arg)> {

    template<typename T, typename Func>
    static Rust::Result<U, E> map(const Rust::Result<T, E>& result, Func func) {
        static_assert(
                std::is_same<T, Arg>::value ||
                std::is_convertible<T, Arg>::value,
                "Incompatible types detected");

        if (result.isOk()) {
            auto res = func(result.storage().template get<T>());
            return res;
        }

        return types::Err<E>(result.storage().template get<E>());
    }
};

// Specialization for a void callback returning a Result
template<typename U, typename E>
struct Map<Rust::Result<U, E> (void)> {

    template<typename T, typename Func>
    static Rust::Result<U, E> map(const Rust::Result<T, E>& result, Func func) {
        static_assert(std::is_same<T, void>::value, "Can not call a void-callback on a non-void Result");

        if (result.isOk()) {
            auto res = func();
            return res;
        }

        return types::Err<E>(result.storage().template get<E>());
    }

};

} // namespace impl

template<typename Func> struct Map : public impl::Map<decltype(&Func::operator())> { };

template<typename Ret, typename... Args>
struct Map<Ret (*) (Args...)> : public impl::Map<Ret (Args...)> { };

template<typename Ret, typename Cls, typename... Args>
struct Map<Ret (Cls::*) (Args...)> : public impl::Map<Ret (Args...)> { };

template<typename Ret, typename Cls, typename... Args>
struct Map<Ret (Cls::*) (Args...) const> : public impl::Map<Ret (Args...)> { };

template<typename Ret, typename... Args>
struct Map<std::function<Ret (Args...)>> : public impl::Map<Ret (Args...)> { };

} // namespace ok


namespace err {

namespace impl {

template<typename T> struct Map;

template<typename Ret, typename Cls, typename Arg>
struct Map<Ret (Cls::*)(Arg) const> {

    static_assert(!IsResult<Ret>::value,
            "Can not map a callback returning a Result, use orElse instead");

    template<typename T, typename E, typename Func>
    static Rust::Result<T, Ret> map(const Rust::Result<T, E>& result, Func func) {
        if (result.isErr()) {
            auto res = func(result.storage().template get<E>());
            return types::Err<Ret>(res);
        }

        return types::Ok<T>(result.storage().template get<T>());
    }

    template<typename E, typename Func>
    static Rust::Result<void, Ret> map(const Rust::Result<void, E>& result, Func func) {
        if (result.isErr()) {
            auto res = func(result.storage().template get<E>());
            return types::Err<Ret>(res);
        }

        return types::Ok<void>();
    }


};

} // namespace impl

template<typename Func> struct Map : public impl::Map<decltype(&Func::operator())> { };

} // namespace err;

namespace And {

namespace impl {

    template<typename Func> struct Then;

    template<typename Ret, typename... Args>
    struct Then<Ret (*)(Args...)> : public Then<Ret (Args...)> { };

    template<typename Ret, typename Cls, typename... Args>
    struct Then<Ret (Cls::*)(Args...)> : public Then<Ret (Args...)> { };

    template<typename Ret, typename Cls, typename... Args>
    struct Then<Ret (Cls::*)(Args...) const> : public Then<Ret (Args...)> { };

    template<typename Ret, typename Arg>
    struct Then<Ret (Arg)> {
        static_assert(std::is_same<Ret, void>::value,
                "then() should not return anything, use map() instead");

        template<typename T, typename E, typename Func>
        static Rust::Result<T, E> then(const Rust::Result<T, E>& result, Func func) {
            if (result.isOk()) {
                func(result.storage().template get<T>());
            }
            return result;
        }
    };

    template<typename Ret>
    struct Then<Ret (void)> {
        static_assert(std::is_same<Ret, void>::value,
                "then() should not return anything, use map() instead");

        template<typename T, typename E, typename Func>
        static Rust::Result<T, E> then(const Rust::Result<T, E>& result, Func func) {
            static_assert(std::is_same<T, void>::value, "Can not call a void-callback on a non-void Result");

            if (result.isOk()) {
                func();
            }

            return result;
        }
    };


} // namespace impl

template<typename Func>
struct Then : public impl::Then<decltype(&Func::operator())> { };

template<typename Ret, typename... Args>
struct Then<Ret (*) (Args...)> : public impl::Then<Ret (Args...)> { };

template<typename Ret, typename Cls, typename... Args>
struct Then<Ret (Cls::*)(Args...)> : public impl::Then<Ret (Args...)> { };

template<typename Ret, typename Cls, typename... Args>
struct Then<Ret (Cls::*)(Args...) const> : public impl::Then<Ret (Args...)> { };

} // namespace And

namespace Or {

namespace impl {

    template<typename Func> struct Else;

    template<typename Ret, typename... Args>
    struct Else<Ret (*)(Args...)> : public Else<Ret (Args...)> { };

    template<typename Ret, typename Cls, typename... Args>
    struct Else<Ret (Cls::*)(Args...)> : public Else<Ret (Args...)> { };

    template<typename Ret, typename Cls, typename... Args>
    struct Else<Ret (Cls::*)(Args...) const> : public Else<Ret (Args...)> { };

    template<typename T, typename F, typename Arg>
    struct Else<Rust::Result<T, F> (Arg)> {

        template<typename E, typename Func>
        static Rust::Result<T, F> orElse(const Rust::Result<T, E>& result, Func func) {
            static_assert(
                    std::is_same<E, Arg>::value ||
                    std::is_convertible<E, Arg>::value,
                    "Incompatible types detected");

            if (result.isErr()) {
                auto res = func(result.storage().template get<E>());
                return res;
            }

            return types::Ok<T>(result.storage().template get<T>());
        }

        template<typename E, typename Func>
        static Rust::Result<void, F> orElse(const Rust::Result<void, E>& result, Func func) {
            if (result.isErr()) {
                auto res = func(result.storage().template get<E>());
                return res;
            }

            return types::Ok<void>();
        }

    };

    template<typename T, typename F>
    struct Else<Rust::Result<T, F> (void)> {

        template<typename E, typename Func>
        static Rust::Result<T, F> orElse(const Rust::Result<T, E>& result, Func func) {
            static_assert(std::is_same<T, void>::value,
                    "Can not call a void-callback on a non-void Result");

            if (result.isErr()) {
                auto res = func();
                return res;
            }

            return types::Ok<T>(result.storage().template get<T>());
        }

        template<typename E, typename Func>
        static Rust::Result<void, F> orElse(const Rust::Result<void, E>& result, Func func) {
            if (result.isErr()) {
                auto res = func();
                return res;
            }

            return types::Ok<void>();
        }

    };

} // namespace impl

template<typename Func>
struct Else : public impl::Else<decltype(&Func::operator())> { };

template<typename Ret, typename... Args>
struct Else<Ret (*) (Args...)> : public impl::Else<Ret (Args...)> { };

template<typename Ret, typename Cls, typename... Args>
struct Else<Ret (Cls::*)(Args...)> : public impl::Else<Ret (Args...)> { };

template<typename Ret, typename Cls, typename... Args>
struct Else<Ret (Cls::*)(Args...) const> : public impl::Else<Ret (Args...)> { };

} // namespace Or

namespace Other {

namespace impl {

    template<typename Func> struct Wise;

    template<typename Ret, typename... Args>
    struct Wise<Ret (*)(Args...)> : public Wise<Ret (Args...)> { };

    template<typename Ret, typename Cls, typename... Args>
    struct Wise<Ret (Cls::*)(Args...)> : public Wise<Ret (Args...)> { };

    template<typename Ret, typename Cls, typename... Args>
    struct Wise<Ret (Cls::*)(Args...) const> : public Wise<Ret (Args...)> { };

    template<typename Ret, typename Arg>
    struct Wise<Ret (Arg)> {

        template<typename T, typename E, typename Func>
        static Rust::Result<T, E> otherwise(const Rust::Result<T, E>& result, Func func) {
            static_assert(
                    std::is_same<E, Arg>::value ||
                    std::is_convertible<E, Arg>::value,
                    "Incompatible types detected");

            static_assert(std::is_same<Ret, void>::value,
                    "callback should not return anything, use mapError() for that");

            if (result.isErr()) {
                func(result.storage().template get<E>());
            }
            return result;
        }

    };

} // namespace impl

template<typename Func>
struct Wise : public impl::Wise<decltype(&Func::operator())> { };

template<typename Ret, typename... Args>
struct Wise<Ret (*) (Args...)> : public impl::Wise<Ret (Args...)> { };

template<typename Ret, typename Cls, typename... Args>
struct Wise<Ret (Cls::*)(Args...)> : public impl::Wise<Ret (Args...)> { };

template<typename Ret, typename Cls, typename... Args>
struct Wise<Ret (Cls::*)(Args...) const> : public impl::Wise<Ret (Args...)> { };

} // namespace Other

template<typename T, typename E, typename Func,
         typename Ret =
            Rust::Result<
                typename details::ResultOkType<
                    typename details::result_of<Func>::type
                >::type,
            E>
        >
Ret map(const Rust::Result<T, E>& result, Func func) {
    return ok::Map<Func>::map(result, func);
}

template<typename T, typename E, typename Func,
         typename Ret =
            Rust::Result<T,
                typename details::ResultErrType<
                    typename details::result_of<Func>::type
                >::type
            >
        >
Ret mapError(const Rust::Result<T, E>& result, Func func) {
    return err::Map<Func>::map(result, func);
}

template<typename T, typename E, typename Func>
Rust::Result<T, E> then(const Rust::Result<T, E>& result, Func func) {
    return And::Then<Func>::then(result, func);
}

template<typename T, typename E, typename Func>
Rust::Result<T, E> otherwise(const Rust::Result<T, E>& result, Func func) {
    return Other::Wise<Func>::otherwise(result, func);
}

template<typename T, typename E, typename Func,
    typename Ret =
        Rust::Result<T,
            typename details::ResultErrType<
                typename details::result_of<Func>::type
            >::type
       >
>
Ret orElse(const Rust::Result<T, E>& result, Func func) {
    return Or::Else<Func>::orElse(result, func);
}

struct ok_tag { };
struct err_tag { };

template<typename T, typename E>
struct Storage {
    static constexpr size_t Size = sizeof(T) > sizeof(E) ? sizeof(T) : sizeof(E);
    static constexpr size_t Align = sizeof(T) > sizeof(E) ? alignof(T) : alignof(E);

    typedef typename std::aligned_storage<Size, Align>::type type;

    Storage()
        : initialized_(false)
    { }

    void construct(types::Ok<T> ok)
    {
        new (&storage_) T(ok.val);
        initialized_ = true;
    }
    void construct(types::Err<E> err)
    {
        new (&storage_) E(err.val);
        initialized_ = true;
    }

    template<typename U>
    void rawConstruct(U&& val) {
        typedef typename std::decay<U>::type CleanU;

        new (&storage_) CleanU(std::forward<U>(val));
        initialized_ = true;
    }

    template<typename U>
    const U& get() const {
        return *reinterpret_cast<const U *>(&storage_);
    }

    template<typename U>
    U& get() {
        return *reinterpret_cast<U *>(&storage_);
    }

    void destroy(ok_tag) {
        if (initialized_) {
            get<T>().~T();
            initialized_ = false;
        }
    }

    void destroy(err_tag) {
        if (initialized_) {
            get<E>().~E();
            initialized_ = false;
        }
    }

    type storage_;
    bool initialized_;
};

template<typename E>
struct Storage<void, E> {
    typedef typename std::aligned_storage<sizeof(E), alignof(E)>::type type;

    void construct(types::Ok<void>)
    {
        initialized_ = true;
    }

    void construct(types::Err<E> err)
    {
        new (&storage_) E(err.val);
        initialized_ = true;
    }

    template<typename U>
    void rawConstruct(U&& val) {
        typedef typename std::decay<U>::type CleanU;

        new (&storage_) CleanU(std::forward<U>(val));
        initialized_ = true;
    }

    void destroy(ok_tag) { initialized_ = false; }
    void destroy(err_tag) {
        if (initialized_) {
            get<E>().~E(); initialized_ = false;
        }
    }

    template<typename U>
    const U& get() const {
        return *reinterpret_cast<const U *>(&storage_);
    }

    template<typename U>
    U& get() {
        return *reinterpret_cast<U *>(&storage_);
    }

    type storage_;
    bool initialized_;
};

template<typename T, typename E>
struct Constructor {

    static void move(Storage<T, E>&& src, Storage<T, E>& dst, ok_tag) {
        dst.rawConstruct(std::move(src.template get<T>()));
        src.destroy(ok_tag());
    }

    static void copy(const Storage<T, E>& src, Storage<T, E>& dst, ok_tag) {
        dst.rawConstruct(src.template get<T>());
    }

    static void move(Storage<T, E>&& src, Storage<T, E>& dst, err_tag) {
        dst.rawConstruct(std::move(src.template get<E>()));
        src.destroy(err_tag());
    }

    static void copy(const Storage<T, E>& src, Storage<T, E>& dst, err_tag) {
        dst.rawConstruct(src.template get<E>());
    }
};

template<typename E>
struct Constructor<void, E> {
    static void move(Storage<void, E>&& src, Storage<void, E>& dst, ok_tag) {
    }

    static void copy(const Storage<void, E>& src, Storage<void, E>& dst, ok_tag) {
    }

    static void move(Storage<void, E>&& src, Storage<void, E>& dst, err_tag) {
        dst.rawConstruct(std::move(src.template get<E>()));
        src.destroy(err_tag());
    }

    static void copy(const Storage<void, E>& src, Storage<void, E>& dst, err_tag) {
        dst.rawConstruct(src.template get<E>());
    }
};

} // namespace details

namespace rpog {

template<typename T, typename = void> struct EqualityComparable : std::false_type { };

template<typename T>
struct EqualityComparable<T,
typename std::enable_if<
    true,
    typename details::void_t<decltype(std::declval<T>() == std::declval<T>())>::type
    >::type
> : std::true_type
{
};


} // namespace rpog

namespace Rust {
template<typename T, typename E>
struct Result {

    static_assert(!std::is_same<E, void>::value, "void error type is not allowed");

    typedef details::Storage<T, E> storage_type;

    Result(types::Ok<T> ok)
        : ok_(true)
    {
        storage_.construct(std::move(ok));
    }

    Result(types::Err<E> err)
        : ok_(false)
    {
        storage_.construct(std::move(err));
    }

    Result(Result&& other) {
        if (other.isOk()) {
            details::Constructor<T, E>::move(std::move(other.storage_), storage_, details::ok_tag());
            ok_ = true;
        } else {
            details::Constructor<T, E>::move(std::move(other.storage_), storage_, details::err_tag());
            ok_ = false;
        }
    }

    Result(const Result& other) {
        if (other.isOk()) {
            details::Constructor<T, E>::copy(other.storage_, storage_, details::ok_tag());
            ok_ = true;
        } else {
            details::Constructor<T, E>::copy(other.storage_, storage_, details::err_tag());
            ok_ = false;
        }
    }

    ~Result() {
        if (ok_)
            storage_.destroy(details::ok_tag());
        else
            storage_.destroy(details::err_tag());
    }

    bool isOk() const {
        return ok_;
    }

    bool isErr() const {
        return !ok_;
    }

    T expect(const char* str) const {
        if (!isOk()) {
            std::fprintf(stderr, "%s\n", str);
            std::terminate(); 
        }
        return expect_impl(std::is_same<T, void>());
    }

    template<typename Func,
             typename Ret =
                Result<
                    typename details::ResultOkType<
                        typename details::result_of<Func>::type
                    >::type,
                E>
            >
    Ret map(Func func) const {
        return details::map(*this, func);
    }

    template<typename Func,
         typename Ret =
             Result<T,
                typename details::ResultErrType<
                    typename details::result_of<Func>::type
                >::type
            >
    >
    Ret mapError(Func func) const {
        return details::mapError(*this, func);
    }

    template<typename Func>
    Result<T, E> then(Func func) const {
        return details::then(*this, func);
    }

    template<typename Func>
    Result<T, E> otherwise(Func func) const {
        return details::otherwise(*this, func);
    }

    template<typename Func,
        typename Ret =
            Result<T,
                typename details::ResultErrType<
                    typename details::result_of<Func>::type
                >::type
           >
    >
    Ret orElse(Func func) const {
        return details::orElse(*this, func);
    }

    storage_type& storage() {
        return storage_;
    }

    const storage_type& storage() const {
        return storage_;
    }

    template<typename U = T>
    typename std::enable_if<
        !std::is_same<U, void>::value,
        U
    >::type
    unwrapOr(const U& defaultValue) const {
        if (isOk()) {
            return storage().template get<U>();
        }
        return defaultValue;
    }

    template<typename U = T>
    typename std::enable_if<
        !std::is_same<U, void>::value,
        U
    >::type
    unwrap() const {
        if (isOk()) {
            return storage().template get<U>();
        }

        std::fprintf(stderr, "Attempting to unwrap an error Result\n");
        std::terminate();
    }

    E unwrapErr() const {
        if (isErr()) {
            return storage().template get<E>();
        }

        std::fprintf(stderr, "Attempting to unwrapErr an ok Result\n");
        std::terminate();
    }

private:
    T expect_impl(std::true_type) const { }
    T expect_impl(std::false_type) const { return storage_.template get<T>(); }

    bool ok_;
    storage_type storage_;
};

template<typename T, typename E>
bool operator==(const Rust::Result<T, E>& lhs, const Rust::Result<T, E>& rhs) {
    static_assert(rpog::EqualityComparable<T>::value, "T must be EqualityComparable for Result to be comparable");
    static_assert(rpog::EqualityComparable<E>::value, "E must be EqualityComparable for Result to be comparable");

    if (lhs.isOk() && rhs.isOk()) {
        return lhs.storage().template get<T>() == rhs.storage().template get<T>();
    }
    if (lhs.isErr() && rhs.isErr()) {
        return lhs.storage().template get<E>() == rhs.storage().template get<E>();
    }
}

template<typename T, typename E>
bool operator==(const Rust::Result<T, E>& lhs, types::Ok<T> ok) {
    static_assert(rpog::EqualityComparable<T>::value, "T must be EqualityComparable for Result to be comparable");

    if (!lhs.isOk()) return false;

    return lhs.storage().template get<T>() == ok.val;
}

template<typename E>
bool operator==(const Rust::Result<void, E>& lhs, types::Ok<void>) {
    return lhs.isOk();
}

template<typename T, typename E>
bool operator==(const Rust::Result<T, E>& lhs, types::Err<E> err) {
    static_assert(rpog::EqualityComparable<E>::value, "E must be EqualityComparable for Result to be comparable");
    if (!lhs.isErr()) return false;

    return lhs.storage().template get<E>() == err.val;
}
} // end namespace Rust

#define TRY(...)                                                   \
    ({                                                             \
        auto res = __VA_ARGS__;                                    \
        if (!res.isOk()) {                                         \
            typedef details::ResultErrType<decltype(res)>::type E; \
            return types::Err<E>(res.storage().get<E>());          \
        }                                                          \
        typedef details::ResultOkType<decltype(res)>::type T;      \
        res.storage().get<T>();                                    \
    })
