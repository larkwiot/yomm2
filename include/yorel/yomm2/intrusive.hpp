#ifndef YOREL_YOMM2_INTRUSIVE_INCLUDED
#define YOREL_YOMM2_INTRUSIVE_INCLUDED

#include <yorel/yomm2/core.hpp>

namespace yorel {
namespace yomm2 {

struct direct;
struct indirect;

template<
    class Class, class Indirection = direct,
    class Policy = policy::default_policy>
struct base;

template<class Class, class Policy>
struct base<Class, direct, Policy> {
    mptr_type YoMm2_S_mptr_{method_table<Class, Policy>};

    mptr_type yomm2_mptr() const {
        return YoMm2_S_mptr_;
    };

    void yomm2_mptr(mptr_type mptr) {
        YoMm2_S_mptr_ = mptr;
    };

    static Policy yomm2_mptr(int);
};

template<class Class, class Policy>
struct base<Class, indirect, Policy> {
    mptr_type* YoMm2_S_mptr_{&method_table<Class, Policy>};

    mptr_type* yomm2_mptr() const {
        return YoMm2_S_mptr_;
    };

    void yomm2_mptr(mptr_type* mptr) {
        YoMm2_S_mptr_ = mptr;
    };

    static Policy yomm2_mptr(int);
};

template<class...>
struct derived;

template<class Class>
struct derived<Class> {
    derived() {
        if constexpr (detail::has_direct_mptr_v<Class>) {
            static_cast<Class*>(this)->yomm2_mptr(
                method_table<Class, decltype(Class::yomm2_mptr(int()))>);
        } else {
            static_assert(detail::has_indirect_mptr_v<Class>);
            static_cast<Class*>(this)->yomm2_mptr(
                &method_table<Class, decltype(Class::yomm2_mptr(int()))>);
        }
    }
};

template<class Class, class Base1, class... Bases>
struct derived<Class, Base1, Bases...> {
    derived() {
        if constexpr (detail::has_direct_mptr_v<Base1>) {
            yomm2_mptr(method_table<Class, decltype(Base1::yomm2_mptr(int()))>);
        } else {
            static_assert(detail::has_indirect_mptr_v<Base1>);
            yomm2_mptr(
                &method_table<Class, decltype(Base1::yomm2_mptr(int()))>);
        }
    }

    auto yomm2_mptr() const {
        return static_cast<const Class*>(this)->Base1::yomm2_mptr();
    };

    void yomm2_mptr(std::conditional_t<
                    detail::has_direct_mptr_v<Base1>, mptr_type, mptr_type*>
                        mptr) {
        auto this_ = static_cast<Class*>(this);
        static_cast<Base1*>(this_)->yomm2_mptr(mptr);
        (static_cast<Bases*>(this_)->yomm2_mptr(mptr), ...);
    }

    static decltype(Base1::yomm2_mptr(int())) yomm2_mptr(int);
};

template<typename Class>
constexpr bool is_aware =
    detail::has_direct_mptr_v<Class> || detail::has_indirect_mptr_v<Class>;

template<bool IsAware>
struct aware_impl;

template<>
struct aware_impl<false> {
    template<
        typename Class, typename Indirection = direct,
        typename Policy = policy::default_policy>
    using crtp = base<Class, Indirection, Policy>;
};

template<>
struct aware_impl<true> {
    template<typename Class>
    using crtp = derived<Class>;
};

template<class Class, typename... Rest>
using aware =
    typename aware_impl<is_aware<Class>>::template crtp<Class, Rest...>;

} // namespace yomm2
} // namespace yorel

#endif
