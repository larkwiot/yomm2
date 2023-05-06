#ifndef YOREL_YOMM2_DETAIL_INCLUDED
#define YOREL_YOMM2_DETAIL_INCLUDED

#include "yorel/yomm2/detail/chain.hpp"

namespace yorel {
namespace yomm2 {
namespace detail {

using mptr_type = detail::word*;

template<typename, typename = default_policy>
mptr_type method_table;

template<typename, typename = default_policy>
mptr_type* indirect_method_table;

struct yomm2_end_of_dump {};

template<typename T>
struct dump_type {
    static_assert(std::is_same_v<T, yomm2_end_of_dump>);
};

struct runtime;

yOMM2_API void update_methods(catalog& cat, context& ht);

namespace mp11 = boost::mp11;

extern yOMM2_API error_handler_type error_handler;

#ifdef NDEBUG
constexpr bool debug = false;
#else
constexpr bool debug = true;
#endif

enum { TRACE_RUNTIME = 1, TRACE_CALLS = 2 };
yOMM2_API extern std::ostream* logs;
yOMM2_API extern unsigned trace_flags;

#if defined(YOMM2_ENABLE_TRACE)
constexpr unsigned trace_enabled = YOMM2_ENABLE_TRACE;
#elif !defined(NDEBUG)
constexpr unsigned trace_enabled = TRACE_RUNTIME;
#else
constexpr unsigned trace_enabled = 0;
#endif

template<unsigned Flags>
struct trace_type {
    trace_type& operator++();
    int indent{0};
};

inline trace_type<TRACE_CALLS> call_trace;

template<typename T, unsigned Flags>
inline trace_type<Flags>& operator<<(trace_type<Flags>& trace, T&& value) {
    if constexpr (bool(trace_enabled & Flags)) {
        if (trace_flags & Flags) {
            *logs << value;
        }
    }
    return trace;
}

inline word make_word(size_t i) {
    word w;
    w.i = i;
    return w;
}

inline word make_word(void* pf) {
    word w;
    w.pf = pf;
    return w;
}

template<auto Function, typename Type = decltype(Function)>
struct parameter_type_list;

template<auto Function, typename ReturnType, typename... ParameterTypes>
struct parameter_type_list<Function, ReturnType (*)(ParameterTypes...)> {
    using type = types<ParameterTypes...>;
};

template<auto Function, typename Type = decltype(Function)>
using parameter_type_list_t =
    typename parameter_type_list<Function, Type>::type;

template<typename T>
struct is_virtual : std::false_type {};

template<typename T>
struct is_virtual<virtual_<T>> : std::true_type {};

template<typename T, class Policy>
struct is_virtual<virtual_ptr<T, Policy>> : std::true_type {};

template<typename T>
struct remove_virtual_ {
    using type = T;
};

template<typename T>
struct remove_virtual_<virtual_<T>> {
    using type = T;
};

template<typename T>
using remove_virtual = typename remove_virtual_<T>::type;

void type_of_name(...);

template<typename Container>
auto type_of_name(Container t) -> decltype(t.name());

template<typename Container>
using type_of_name_t = decltype(type_of_name(std::declval<Container>()));

template<typename Container>
constexpr bool has_name_v =
    std::is_same_v<type_of_name_t<Container>, std::string_view>;

// ----------
// has_next_v

void type_next(...);

template<typename Container>
auto type_next(Container t) -> decltype(t.next);

template<typename Container>
using type_next_t = decltype(type_next(std::declval<Container>()));

template<typename Container, typename Next>
constexpr bool has_next_v = std::is_same_v<type_next_t<Container>, Next>;

// --------------
// intrusive mode

void type_mptr(...);

template<typename Object>
auto type_mptr(Object* obj) -> decltype(obj->yomm2_mptr());

template<typename Object>
using type_mptr_t = decltype(type_mptr(std::declval<Object*>()));

template<typename Object>
constexpr bool has_direct_mptr_v = std::is_same_v<type_mptr_t<Object>, word*>;

template<typename Object>
constexpr bool has_indirect_mptr_v =
    std::is_same_v<type_mptr_t<Object>, word**>;

// -----------
// virtual_ptr

template<typename>
struct is_virtual_ptr_aux : std::false_type {};

template<class Class, class Policy>
struct is_virtual_ptr_aux<virtual_ptr<Class, Policy>> : std::true_type {};

template<typename T>
constexpr bool is_virtual_ptr = is_virtual_ptr_aux<T>::value;

// -------------
// hash function

inline std::size_t hash(std::uintptr_t mult, std::size_t shift, const void* p) {
    return static_cast<std::size_t>(
        (mult * reinterpret_cast<std::uintptr_t>(const_cast<void*>(p))) >>
        shift);
}

// ----------
// class info

struct class_info : static_chain<class_info>::static_link {
    detail::ti_ptr ti;
    word** intrusive_mptr;
    const detail::ti_ptr *first_base, *last_base;
    const char* name() const {
        return ti->name();
    }
    bool is_abstract{false};
};

// -----------
// method info

struct method_info;

struct definition_info : static_chain<definition_info>::static_link {
    ~definition_info();
    method_info* method;
    std::string_view name;
    void** next;
    const std::type_info *const *vp_begin, *const *vp_end;
    void* pf;
};

struct yOMM2_API method_info : static_chain<method_info>::static_link {
    std::string_view name;
    const std::type_info *const *vp_begin, *const *vp_end;
    static_chain<definition_info> specs;
    void* ambiguous;
    void* not_implemented;
    const std::type_info* hash_factors_placement;
    size_t* slots_strides_p;

    virtual void install_hash_factors(runtime&);

    auto arity() const {
        return std::distance(vp_begin, vp_end);
    }
};

inline definition_info::~definition_info() {
    if (method) {
        method->specs.remove(*this);
    }
}

template<typename T>
constexpr bool is_policy = std::is_base_of_v<policy::abstract_policy, T>;

template<bool, typename... Classes>
struct split_policy_aux;

template<typename Policy, typename... Classes>
struct split_policy_aux<true, Policy, Classes...> {
    using policy = Policy;
    using classes = types<Classes...>;
};

template<typename... Classes>
struct split_policy_aux<false, Classes...> {
    using policy = default_policy;
    using classes = types<Classes...>;
};

template<typename ClassOrPolicy, typename... Classes>
struct split_policy
    : split_policy_aux<is_policy<ClassOrPolicy>, ClassOrPolicy, Classes...> {
};

template<typename... Classes>
using get_policy = typename split_policy<Classes...>::policy;

template<typename... Classes>
using remove_policy = typename split_policy<Classes...>::classes;

template<typename Signature>
struct next_ptr_t;

template<typename R, typename... T>
struct next_ptr_t<R(T...)> {
    using type = R (*)(T...);
};

template<typename Method, typename Signature>
inline typename next_ptr_t<Signature>::type next;

template<typename B, typename D, typename = void>
struct optimal_cast_impl {
    static constexpr bool requires_dynamic_cast = true;
    static D fn(B obj) {
        return dynamic_cast<D>(obj);
    }
};

template<typename B, typename D>
struct optimal_cast_impl<
    B, D, std::void_t<decltype(static_cast<D>(std::declval<B>()))>> {
    static constexpr bool requires_dynamic_cast = false;
    static D fn(B obj) {
        return static_cast<D>(obj);
    }
};

template<class D, class B>
decltype(auto) optimal_cast(B&& obj) {
    return optimal_cast_impl<B, D>::fn(std::forward<B>(obj));
}

template<class D, class B>
constexpr bool requires_dynamic_cast =
    optimal_cast_impl<B, D>::requires_dynamic_cast;

template<typename T>
struct virtual_traits;

template<typename T>
struct virtual_traits<virtual_<T>> : virtual_traits<T> {};

template<typename T>
using polymorphic_type = typename virtual_traits<T>::polymorphic_type;

template<typename T>
struct virtual_traits<T&> {
    using polymorphic_type = std::remove_cv_t<T>;
    static_assert(std::is_polymorphic_v<polymorphic_type>);

    static const T& rarg(const T& arg) {
        return arg;
    }

    static auto key(const T& arg) {
        return &typeid(arg);
    }

    template<typename D>
    static D& cast(T& obj) {
        return optimal_cast<D&>(obj);
    }
};

template<typename T>
struct virtual_traits<T&&> {
    using polymorphic_type = std::remove_cv_t<T>;
    static_assert(std::is_polymorphic_v<polymorphic_type>);

    static const T& rarg(const T& arg) {
        return arg;
    }

    static auto key(const T& arg) {
        return &typeid(arg);
    }

    template<typename D>
    static D&& cast(T&& obj) {
        return optimal_cast<D&&>(obj);
    }
};

template<typename T>
struct virtual_traits<T*> {
    using polymorphic_type = std::remove_cv_t<T>;
    static_assert(std::is_polymorphic_v<polymorphic_type>);

    static auto rarg(const T* arg) {
        return arg;
    }

    static auto key(const T* arg) {
        return &typeid(*arg);
    }

    template<typename D>
    static D cast(T* obj) {
        return optimal_cast<D>(obj);
    }
};

template<class Class, class Policy>
struct virtual_traits<virtual_ptr<Class, Policy>> {
    using polymorphic_type = Class;
    static_assert(std::is_polymorphic_v<polymorphic_type>);

    static auto rarg(virtual_ptr<Class, Policy> ptr) {
        return ptr;
    }

    static auto key(virtual_ptr<Class, Policy> arg) {
        return &typeid(*arg);
    }

    template<typename Derived>
    static Derived cast(virtual_ptr<Class, Policy> ptr) {
        return ptr.template cast<Derived>();
    }
};

template<typename T>
struct resolver_type_impl {
    using type = const T&;
};

template<typename T>
struct resolver_type_impl<T*> {
    using type = const T*;
};

template<typename T>
struct resolver_type_impl<T&> {
    using type = const T&;
};

template<typename T>
struct resolver_type_impl<T&&> {
    using type = const T&;
};

template<typename T>
struct resolver_type_impl<virtual_<T>> {
    using type = decltype(virtual_traits<T>::rarg(std::declval<T>()));
};

template<class Class, class Policy>
struct resolver_type_impl<virtual_ptr<Class, Policy>> {
    using type = virtual_ptr<Class, Policy>;
};

template<typename T>
using resolver_type = typename resolver_type_impl<T>::type;

template<typename T>
struct argument_traits {
    static const T& rarg(const T& arg) {
        return arg;
    }

    static auto key(const T& arg) {
        return &typeid(arg);
    }

#if defined(_MSC_VER) && (_MSC_VER / 100) <= 19

    template<typename U>
    static U& cast(U& obj) {
        return obj;
    }

    template<typename U>
    static U&& cast(U&& obj) {
        return obj;
    }

#else

    template<typename U>
    static decltype(auto) cast(U&& obj) {
        return std::forward<U>(obj);
    }

#endif
};

template<typename T>
struct argument_traits<virtual_<T>> : virtual_traits<T> {};

template<class Class, class Policy>
struct argument_traits<virtual_ptr<Class, Policy>>
    : virtual_traits<virtual_ptr<Class, Policy>> {};

template<typename T>
struct shared_ptr_traits {
    static const bool is_shared_ptr = false;
};

template<typename T>
struct shared_ptr_traits<std::shared_ptr<T>> {
    static const bool is_shared_ptr = true;
    static const bool is_const_ref = false;
    using polymorphic_type = T;
};

template<typename T>
struct shared_ptr_traits<const std::shared_ptr<T>&> {
    static const bool is_shared_ptr = true;
    static const bool is_const_ref = true;
    using polymorphic_type = T;
};

template<typename T>
struct virtual_traits<std::shared_ptr<T>> {
    using polymorphic_type = std::remove_cv_t<T>;
    static_assert(std::is_polymorphic_v<polymorphic_type>);

    static const std::shared_ptr<T>& rarg(const std::shared_ptr<T>& arg) {
        return arg;
    }

    static auto key(const std::shared_ptr<T>& arg) {
        return &typeid(*arg);
    }

    template<class DERIVED>
    static void check_cast() {
        static_assert(shared_ptr_traits<DERIVED>::is_shared_ptr);
        static_assert(
            !shared_ptr_traits<DERIVED>::is_const_ref,
            "cannot cast from 'const shared_ptr<base>&' to "
            "'shared_ptr<derived>'");
        static_assert(std::is_class_v<
                      typename shared_ptr_traits<DERIVED>::polymorphic_type>);
    }
    template<class DERIVED>
    static auto cast(const std::shared_ptr<T>& obj) {
        check_cast<DERIVED>();

        if constexpr (requires_dynamic_cast<T*, DERIVED>) {
            return std::dynamic_pointer_cast<
                typename shared_ptr_traits<DERIVED>::polymorphic_type>(obj);
        } else {
            return std::static_pointer_cast<
                typename shared_ptr_traits<DERIVED>::polymorphic_type>(obj);
        }
    }
};

template<typename T>
struct virtual_traits<const std::shared_ptr<T>&> {
    using polymorphic_type = std::remove_cv_t<T>;
    static_assert(std::is_polymorphic_v<polymorphic_type>);

    static const std::shared_ptr<T>& rarg(const std::shared_ptr<T>& arg) {
        return arg;
    }

    static auto key(const std::shared_ptr<T>& arg) {
        return &typeid(*arg);
    }

    template<class DERIVED>
    static void check_cast() {
        static_assert(shared_ptr_traits<DERIVED>::is_shared_ptr);
        static_assert(
            shared_ptr_traits<DERIVED>::is_const_ref,
            "cannot cast from 'const shared_ptr<base>&' to "
            "'shared_ptr<derived>'");
        static_assert(std::is_class_v<
                      typename shared_ptr_traits<DERIVED>::polymorphic_type>);
    }

    template<class DERIVED>
    static auto cast(const std::shared_ptr<T>& obj) {
        check_cast<DERIVED>();

        if constexpr (requires_dynamic_cast<T*, DERIVED>) {
            return std::dynamic_pointer_cast<
                typename shared_ptr_traits<DERIVED>::polymorphic_type>(obj);
        } else {
            return std::static_pointer_cast<
                typename shared_ptr_traits<DERIVED>::polymorphic_type>(obj);
        }
    }
};

template<typename MethodArgList>
using polymorphic_types = mp11::mp_transform<
    remove_virtual, mp11::mp_filter<detail::is_virtual, MethodArgList>>;

template<typename P, typename Q>
struct select_spec_polymorphic_type {
    using type = void;
};

template<typename P, typename Q>
struct select_spec_polymorphic_type<virtual_<P>, Q> {
    using type = polymorphic_type<Q>;
};

template<typename P, typename Q, class Policy>
struct select_spec_polymorphic_type<
    virtual_ptr<P, Policy>, virtual_ptr<Q, Policy>> {
    using type =
        typename virtual_traits<virtual_ptr<Q, Policy>>::polymorphic_type;
};

template<typename MethodArgList, typename SpecArgList>
using spec_polymorphic_types = mp11::mp_rename<
    mp11::mp_remove<
        mp11::mp_transform_q<
            mp11::mp_quote_trait<select_spec_polymorphic_type>, MethodArgList,
            SpecArgList>,
        void>,
    types>;

template<typename...>
struct type_id_list;

template<typename... T>
struct type_id_list<types<T...>> {
    static constexpr const std::type_info* value[] = {&typeid(T)...};
    static constexpr auto begin = value;
    static constexpr auto end = value + sizeof...(T);
};

template<>
struct type_id_list<types<>> {
    static constexpr const std::type_info** begin = nullptr;
    static constexpr auto end = begin;
};

template<typename ArgType, typename T>
inline auto get_tip(const T& arg) {
    if constexpr (is_virtual<ArgType>::value) {
        return virtual_traits<ArgType>::key(arg);
    } else {
        return &typeid(arg);
    }
}

template<class Policy>
inline auto check_method_pointer(const word* mptr, ti_ptr key) {
    auto& ctx = Policy::context;
    if constexpr (Policy::enable_runtime_checks) {
        auto p = reinterpret_cast<const char*>(mptr);

        if (p < reinterpret_cast<const char*>(ctx.gv.data()) ||
            p >= reinterpret_cast<const char*>(ctx.gv.data() + ctx.gv.size())) {
            error_handler(method_table_error{key});
        }

        auto index = ctx.hash(key);

        if (index >= ctx.mptrs.size() || mptr != ctx.mptrs[index]) {
            error_handler(method_table_error{key});
        }
    }

    return mptr;
}

template<typename Method, typename ArgType>
inline auto get_mptr(resolver_type<ArgType> arg) {
    const word* mptr;
    using policy = typename Method::policy_type;

    if constexpr (has_indirect_mptr_v<ArgType>) {
        mptr = *arg.yomm2_mptr();
        check_method_pointer<policy>(mptr, virtual_traits<ArgType>::key(arg));
    } else if constexpr (has_direct_mptr_v<ArgType>) {
        mptr = arg.yomm2_mptr();
        check_method_pointer<policy>(mptr, virtual_traits<ArgType>::key(arg));
    } else if constexpr (is_virtual_ptr<ArgType>) {
        mptr = arg.method_table();
        check_method_pointer<policy>(mptr, virtual_traits<ArgType>::key(arg));
    } else {
        auto key = virtual_traits<ArgType>::key(arg);

        if constexpr (bool(trace_enabled & TRACE_CALLS)) {
            call_trace << "  key = " << key;
        }

        mptr = policy::context.mptrs[policy::template hash<Method>()(key)];
    }

    if constexpr (bool(trace_enabled & TRACE_CALLS)) {
        call_trace << " mptr = " << mptr;
    }

    return mptr;
}

// -------
// wrapper

template<typename, auto, typename>
struct wrapper;

template<
    typename BASE_RETURN, typename... BASE_PARAM, auto SPEC,
    typename... SPEC_PARAM>
struct wrapper<BASE_RETURN(BASE_PARAM...), SPEC, types<SPEC_PARAM...>> {
    static BASE_RETURN fn(remove_virtual<BASE_PARAM>... arg) {
        using base_type = mp11::mp_first<types<BASE_PARAM...>>;
        using spec_type = mp11::mp_first<types<SPEC_PARAM...>>;
        return SPEC(argument_traits<BASE_PARAM>::template cast<SPEC_PARAM>(
            remove_virtual<BASE_PARAM>(arg))...);
    }
};

template<typename Method, typename Container>
struct next_aux {
    static typename Method::next_type next;
};

template<typename Method, typename Container>
typename Method::next_type next_aux<Method, Container>::next;

template<auto F, typename T>
struct member_function_wrapper;

template<auto F, class R, class C, typename... Args>
struct member_function_wrapper<F, R (C::*)(Args...)> {
    static R fn(C* this_, Args&&... args) {
        return (this_->*F)(args...);
    }
};

// Collect the base classes of a list of classes. The result is a mp11 map that
// associates each class to a list starting with the class itself, followed by
// all its bases, as per std::is_base_of. Thus the list includes the class
// itself at least twice: at the front, and down the list, as its own improper
// base. The direct and its direct and indirect proper bases are included. The
// runtime will extract the direct proper bases. See unit tests for an example.
template<typename... Cs>
using inheritance_map = types<mp11::mp_push_front<
    mp11::mp_filter_q<mp11::mp_bind_back<std::is_base_of, Cs>, types<Cs...>>,
    Cs>...>;

template<typename... Classes>
struct use_classes_aux {
    using type = mp11::mp_apply<
        std::tuple,
        mp11::mp_transform_q<
            mp11::mp_bind_back<class_declaration, get_policy<Classes...>>,
            mp11::mp_apply<inheritance_map, remove_policy<Classes...>>>>;
};

template<typename... Classes, typename... ClassLists>
struct use_classes_aux<types<Classes...>, ClassLists...>
    : mp11::mp_apply<
          use_classes_aux, mp11::mp_append<types<Classes...>, ClassLists...>> {
};

std::ostream* log_on(std::ostream* os);
std::ostream* log_off();

} // namespace detail
} // namespace yomm2
} // namespace yorel

#endif
