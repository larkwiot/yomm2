#ifndef YOREL_YOMM2_CORE_INCLUDED
#define YOREL_YOMM2_CORE_INCLUDED

#include <array>
#include <charconv>
#include <chrono>
#include <stdio.h>
#include <memory>
#include <string_view>
#include <variant>
#include <vector>

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/bind.hpp>

#if defined(YOMM2_SHARED)
    #if !defined(yOMM2_API)
        #if defined(_WIN32)
            #define yOMM2_API __declspec(dllimport)
        #else
            #define yOMM2_API
        #endif
    #endif
#else
    #define yOMM2_API
#endif

// -----------------------------------------------------------------------------
// A few details...

namespace yorel {
namespace yomm2 {

namespace detail {

union word {
    void* pf;
    const word* pw;
    size_t i;
    const void* ti;
};

using mptr_type = detail::word*;
using ti_ptr = const std::type_info*;

} // namespace detail

// -----------------------------------------------------------------------------
// Policies

struct context;
struct catalog;

namespace policy {

struct abstract_policy {
    static constexpr bool use_indirect_method_pointers = false;
    static constexpr bool enable_runtime_checks = false;
    static constexpr bool runtime_checks = false;
};

struct debug;
struct release;
struct debug_shared;
struct release_shared;

} // namespace policy

#if defined(YOMM2_SHARED)
    #ifdef NDEBUG
using default_policy = policy::release_shared;
    #else
using default_policy = policy::debug_shared;
    #endif
#else
    #ifdef NDEBUG
using default_policy = policy::release;
    #else
using default_policy = policy::debug;
    #endif
#endif

// -----------------------------------------------------------------------------
// Error handling

struct resolution_error {
    enum status_type { no_definition = 1, ambiguous } status;
    const std::type_info* method;
    size_t arity;
    const std::type_info* const* tis;
};

struct unknown_class_error {
    enum { update = 1, call } context;
    const std::type_info* ti;
};

struct hash_search_error {
    size_t attempts;
    std::chrono::duration<double> duration;
    size_t buckets;
};

struct method_table_error {
    const std::type_info* ti;
};

using error_type = std::variant<
    resolution_error, unknown_class_error, hash_search_error,
    method_table_error>;

using error_handler_type = void (*)(const error_type& error);
error_handler_type set_error_handler(error_handler_type handler);

// deprecated

struct method_call_error {
    resolution_error::status_type code;
    static constexpr auto not_implemented = resolution_error::no_definition;
    static constexpr auto ambiguous = resolution_error::ambiguous;
    std::string_view method_name;
};

using method_call_error_handler = void (*)(
    const method_call_error& error, size_t arity,
    const std::type_info* const tis[]);

// end deprecated

using error_type = std::variant<
    resolution_error, unknown_class_error, hash_search_error,
    method_table_error>;

using error_handler_type = void (*)(const error_type& error);
error_handler_type set_error_handler(error_handler_type handler);

struct method_call_error;

using method_call_error_handler = void (*)(
    const method_call_error& error, size_t arity,
    const std::type_info* const tis[]);

inline method_call_error_handler yOMM2_API
set_method_call_error_handler(method_call_error_handler handler);

// -----------------------------------------------------------------------------
// Forward declarations needed by "detail.hpp"

template<typename T>
struct virtual_;

template<class Class, class Policy, bool IsSmartPtr>
class virtual_ptr;

template<typename Key, typename Signature, class Policy>
struct method;

template<typename Class, typename... Rest>
struct class_declaration;

} // namespace yomm2
} // namespace yorel

#include "detail.hpp"

namespace yorel {
namespace yomm2 {

// -----------------------------------------------------------------------------
// Scope

struct context {
    std::vector<detail::word> gv;
    std::vector<detail::word*> mptrs;
    std::vector<detail::word**> indirect_mptrs;
    detail::hash_function hash;
    std::vector<detail::ti_ptr> control;
};

struct catalog {
    detail::static_chain<detail::class_info> classes;
    detail::static_chain<detail::method_info> methods;
};

// -----------------------------------------------------------------------------
// Method

template<typename Key, typename Signature, class Policy = default_policy>
struct method;

template<typename Key, typename R, typename... A, class Policy>
struct method<Key, R(A...), Policy> : detail::method_info {
    using self_type = method;
    using policy_type = Policy;
    using declared_argument_types = detail::types<A...>;
    using call_argument_types = boost::mp11::mp_transform<
        detail::remove_virtual, declared_argument_types>;
    using virtual_argument_types =
        typename detail::polymorphic_types<declared_argument_types>;
    using signature_type = R(A...);
    using return_type = R;
    using function_pointer_type = R (*)(detail::remove_virtual<A>...);
    using next_type = function_pointer_type;

    static constexpr auto arity = boost::mp11::mp_count_if<
        declared_argument_types, detail::is_virtual>::value;
    static_assert(arity > 0, "method must have at least one virtual argument");

    size_t slots_strides[arity == 1 ? 1 : 2 * arity - 1];
    // For 1-method: the offset of the method in the method table, which
    // contains a pointer to a function.
    // For multi-methods: the offset of the first virtual argument in the
    // method table, which contains a pointer to the corresponding cell in
    // the dispatch table, followed by the offset of the second argument and
    // the stride in the second dimension, etc.

    static method fn;
    static function_pointer_type fake_definition;

    explicit method(std::string_view name = typeid(method).name());

    method(const method&) = delete;
    method(method&&) = delete;

    ~method();

    template<typename ArgType>
    const detail::word* get_mptr(detail::resolver_type<ArgType> arg) const;

    template<typename ArgType, typename... MoreArgTypes>
    void* resolve_uni(
        detail::resolver_type<ArgType> arg,
        detail::resolver_type<MoreArgTypes>... more_args) const;

    template<size_t VirtualArg, typename ArgType, typename... MoreArgTypes>
    void* resolve_multi_first(
        detail::resolver_type<ArgType> arg,
        detail::resolver_type<MoreArgTypes>... more_args) const;

    template<size_t VirtualArg, typename ArgType, typename... MoreArgTypes>
    void* resolve_multi_next(
        const detail::word* dispatch, detail::resolver_type<ArgType> arg,
        detail::resolver_type<MoreArgTypes>... more_args) const;

    template<typename... ArgType>
    function_pointer_type resolve(detail::resolver_type<ArgType>... args) const;

    return_type operator()(detail::remove_virtual<A>... args) const;

    static return_type
    not_implemented_handler(detail::remove_virtual<A>... args);
    static return_type ambiguous_handler(detail::remove_virtual<A>... args);

    template<typename Container>
    using next = detail::next_aux<method, Container>;

    template<auto Function>
    struct add_function {
        explicit add_function(
            next_type* next = nullptr,
            std::string_view name = typeid(Function).name()) {

            static detail::definition_info info;

            if (info.method) {
                assert(info.method == &fn);
                return;
            }

            info.method = &fn;
            info.name = name;
            info.next = reinterpret_cast<void**>(next);
            using parameter_types = detail::parameter_type_list_t<Function>;
            info.pf = (void*)&detail::wrapper<
                signature_type, Function, parameter_types>::fn;
            using spec_type_ids =
                detail::type_id_list<detail::spec_polymorphic_types<
                    declared_argument_types, parameter_types>>;
            info.vp_begin = spec_type_ids::begin;
            info.vp_end = spec_type_ids::end;
            fn.specs.push_front(info);
        }
    };

    template<auto... Function>
    struct add_functions : std::tuple<add_function<Function>...> {};

    template<typename Container, bool has_next, bool has_name>
    struct add_definition_;

    template<typename Container>
    struct add_definition_<Container, false, false> {
        add_function<Container::fn> override_{
            nullptr, typeid(Container).name()};
    };

    template<typename Container>
    struct add_definition_<Container, true, false> {
        add_function<Container::fn> add{
            &Container::next, typeid(Container).name()};
    };

    template<typename Container>
    struct add_definition_<Container, false, true> {
        add_function<Container::fn> add{nullptr, Container::name};
    };

    template<typename Container>
    struct add_definition_<Container, true, true> {
        add_function<Container::fn> add{&Container::next, Container::name};
    };

    template<typename Container>
    struct add_definition
        : add_definition_<
              Container, detail::has_next_v<Container, next_type>,
              detail::has_name_v<Container>> {
        using type = add_definition; // make it a meta-function
    };

    template<auto F>
    struct add_member_function
        : add_function<detail::member_function_wrapper<F, decltype(F)>::fn> {};

    template<auto... F>
    struct add_member_functions {
        std::tuple<add_member_function<F>...> add;
    };

    template<typename Container>
    struct use_next {
        static next_type next;
    };
};

template<typename Key, typename R, typename... A, class Policy>
method<Key, R(A...), Policy> method<Key, R(A...), Policy>::fn;

template<typename Key, typename R, typename... A, class Policy>
typename method<Key, R(A...), Policy>::function_pointer_type
    method<Key, R(A...), Policy>::fake_definition;

template<typename Key, typename R, typename... A, class Policy>
template<typename Container>
typename method<Key, R(A...), Policy>::next_type
    method<Key, R(A...), Policy>::use_next<Container>::next;

// -----------------------------------------------------------------------------
// class_declaration

template<typename Class, typename... Rest>
struct class_declaration : class_declaration<
                               detail::remove_policy<Class, Rest...>,
                               detail::get_policy<Class, Rest...>> {};

template<typename Class, typename... Bases, typename Policy>
struct class_declaration<detail::types<Class, Bases...>, Policy>
    : detail::class_info {
    class_declaration() {
        using namespace detail;

        ti = &typeid(Class);
        first_base = type_id_list<types<Bases...>>::begin;
        last_base = type_id_list<types<Bases...>>::end;
        Policy::catalog.classes.push_front(*this);
        is_abstract = std::is_abstract_v<Class>;
        method_table = &Policy::template method_table<Class>;
    }

    ~class_declaration() {
        Policy::catalog.classes.remove(*this);
    }
};

template<typename Class, typename... Bases>
struct class_declaration<detail::types<Class, Bases...>>
    : class_declaration<detail::types<Class, Bases...>, default_policy> {};

template<typename... T>
using use_classes = typename detail::use_classes_aux<T...>::type;

// -----------------------------------------------------------------------------
// virtual_ptr

template<
    class Class, class Policy = default_policy,
    bool IsSmartPtr = detail::virtual_ptr_traits<Class, Policy>::is_smart_ptr>
class virtual_ptr;

template<class Class, class Policy, typename Box>
class virtual_ptr_aux {
    template<class, class, typename>
    friend class virtual_ptr_aux;

    template<class, class, bool>
    friend class virtual_ptr;

    template<class, class>
    friend struct detail::virtual_ptr_traits;

    friend struct detail::virtual_traits<virtual_ptr<Class, Policy>>;
    friend struct detail::virtual_traits<const virtual_ptr<Class, Policy>&>;

  protected:
    static constexpr bool is_indirect = Policy::use_indirect_method_pointers;

    using mptr_type =
        std::conditional_t<is_indirect, detail::mptr_type*, detail::mptr_type>;
    using box_type = Box;

    Box obj;
    mptr_type mptr;

  public:
    virtual_ptr_aux(Class& obj, mptr_type mptr) : obj(obj), mptr(mptr) {
    }

    template<class Other>
    static auto final(Other&& obj) {
        using namespace detail;

        mptr_type mptr;

        if constexpr (Policy::use_indirect_method_pointers) {
            mptr = Policy::template indirect_method_table<
                typename detail::virtual_traits<Other&>::polymorphic_type>;
        } else {
            mptr = Policy::template method_table<
                typename detail::virtual_traits<Other&>::polymorphic_type>;
        }

        if constexpr (Policy::runtime_checks) {
            // check that dynamic type == static type
            auto key = virtual_traits<Other&>::key(obj);
            auto final_key =
                &typeid(typename virtual_traits<Other&>::polymorphic_type);

            if (key != final_key) {
                detail::error_handler(method_table_error{key});
            }
        }

        return virtual_ptr<Class, Policy>(obj, mptr);
    }

    template<class Other>
    static auto dynamic_method_table(Other& obj);

    auto& box() const noexcept {
        return obj;
    }

    // consider as private, public for tests only
    auto method_table() const noexcept {
        if constexpr (is_indirect) {
            return *mptr;
        } else {
            return mptr;
        }
    }

  protected:
    virtual_ptr_aux() = default;
};

template<class Class, class Policy>
class virtual_ptr<Class, Policy, false>
    : public virtual_ptr_aux<Class, Policy, Class&> {

    using base = virtual_ptr_aux<Class, Policy, Class&>;

  public:
    template<class Other>
    virtual_ptr(Other& obj) : base(obj, this->dynamic_method_table(obj)) {
    }

    template<typename Other>
    virtual_ptr(Other&& obj, typename base::mptr_type mptr) : base(obj, mptr) {
    }

    template<class Other>
    virtual_ptr(virtual_ptr<Other, Policy>& other)
        : base(other.obj, other.mptr) {
    }

    template<class Other>
    virtual_ptr(const virtual_ptr<Other, Policy>& other)
        : base(other.obj, other.mptr) {
    }

    auto get() const noexcept {
        return &this->obj;
    }

    auto operator->() const noexcept {
        return get();
    }

    auto& operator*() const noexcept {
        return *get();
    }
};

template<class Class, class Policy>
class virtual_ptr<Class, Policy, true>
    : public virtual_ptr_aux<Class, Policy, Class> {

    using base = virtual_ptr_aux<Class, Policy, Class>;

  public:
    template<typename OtherBox>
    virtual_ptr(OtherBox& obj) : base(obj, this->dynamic_method_table(obj)) {
    }

    template<typename OtherBox>
    virtual_ptr(OtherBox&& obj, typename base::mptr_type mptr)
        : base(obj, mptr) {
    }

    template<typename OtherBox>
    virtual_ptr(OtherBox&& other) {
        this->obj = std::move(other);
        this->mptr = this->dynamic_method_table(this->obj);
    }

    template<typename OtherBox>
    virtual_ptr(virtual_ptr<OtherBox, Policy>& other) {
        this->obj = other.obj;
        this->mptr = other.mptr;
    }

    template<typename OtherBox>
    virtual_ptr(const virtual_ptr<OtherBox, Policy>& other) {
        this->obj = other.obj;
        this->mptr = other.mptr;
    }

    template<typename OtherBox>
    virtual_ptr(virtual_ptr<OtherBox, Policy>&& other) {
        this->obj = std::move(other.obj);
        this->mptr = other.mptr;
    }

    auto get() const noexcept {
        return this->obj.get();
    }

    auto operator->() const noexcept {
        return get();
    }

    auto& operator*() const noexcept {
        return *get();
    }
    template<typename OtherPtr>
    auto cast() const {
        return detail::virtual_ptr_traits<Class, Policy>::template cast<
            OtherPtr>(*this);
    }
};

template<class Class>
virtual_ptr(Class&) -> virtual_ptr<
    Class, default_policy,
    detail::virtual_ptr_traits<Class, default_policy>::is_smart_ptr>;

template<class Class, class Policy = default_policy>
using virtual_shared_ptr = virtual_ptr<std::shared_ptr<Class>, Policy>;

template<class Class, class Policy = default_policy>
inline auto make_virtual_shared() {
    return virtual_shared_ptr<Class, Policy>::final(std::make_shared<Class>());
}

// -----------------------------------------------------------------------------
// definitions

template<typename Key, typename R, typename... A, class Policy>
method<Key, R(A...), Policy>::method(std::string_view name) {
    this->name = name;
    this->slots_strides_p = slots_strides;
    using virtual_type_ids = detail::type_id_list<boost::mp11::mp_transform<
        detail::polymorphic_type, virtual_argument_types>>;
    this->vp_begin = virtual_type_ids::begin;
    this->vp_end = virtual_type_ids::end;
    this->not_implemented = (void*)not_implemented_handler;
    this->ambiguous = (void*)ambiguous_handler;
    this->hash_factors_placement = &typeid(Policy);
    Policy::catalog.methods.push_front(*this);
}

template<typename Key, typename R, typename... A, class Policy>
method<Key, R(A...), Policy>::~method() {
    Policy::catalog.methods.remove(*this);
}

template<typename Key, typename R, typename... A, class Policy>
typename method<Key, R(A...), Policy>::return_type inline method<
    Key, R(A...), Policy>::operator()(detail::remove_virtual<A>... args) const {
    using namespace detail;
    return resolve<A...>(argument_traits<A>::rarg(args)...)(
        std::forward<remove_virtual<A>>(args)...);
}

template<typename Key, typename R, typename... A, class Policy>
template<typename... ArgType>
inline typename method<Key, R(A...), Policy>::function_pointer_type
method<Key, R(A...), Policy>::resolve(
    detail::resolver_type<ArgType>... args) const {
    using namespace detail;

    void* pf;

    if constexpr (arity == 1) {
        pf = resolve_uni<ArgType...>(args...);
    } else {
        pf = resolve_multi_first<0, ArgType...>(args...);
    }

    return reinterpret_cast<function_pointer_type>(pf);
}

template<typename Key, typename R, typename... A, class Policy>
template<typename ArgType>
inline const detail::word* method<Key, R(A...), Policy>::get_mptr(
    detail::resolver_type<ArgType> arg) const {
    using namespace detail;

    const word* mptr;

    if constexpr (has_mptr<resolver_type<ArgType>>) {
        mptr = arg.yomm2_mptr();
        check_intrusive_method_pointer<Policy>(
            mptr, virtual_traits<ArgType>::key(arg));
    } else if constexpr (is_virtual_ptr<ArgType>) {
        mptr = arg.method_table();
        // No need to check the method pointer: this was done when the
        // virtual_ptr was created.
    } else {
        auto key = virtual_traits<ArgType>::key(arg);
        auto index = Policy::context.hash(key);

        if constexpr (Policy::runtime_checks) {
            auto control_key = Policy::context.control[index];
            if (control_key != key) {
                error_handler(unknown_class_error{unknown_class_error::call, key});
            }
        }

        mptr = Policy::context.mptrs[index];
    }

    return mptr;
}

template<typename Key, typename R, typename... A, class Policy>
template<typename ArgType, typename... MoreArgTypes>
inline void* method<Key, R(A...), Policy>::resolve_uni(
    detail::resolver_type<ArgType> arg,
    detail::resolver_type<MoreArgTypes>... more_args) const {

    using namespace detail;

    if constexpr (is_virtual<ArgType>::value) {
        const word* mptr = get_mptr<ArgType>(arg);
        return mptr[this->slots_strides[0]].pf;
    } else {
        return resolve_uni<MoreArgTypes...>(more_args...);
    }
}

template<typename Key, typename R, typename... A, class Policy>
template<size_t VirtualArg, typename ArgType, typename... MoreArgTypes>
inline void* method<Key, R(A...), Policy>::resolve_multi_first(
    detail::resolver_type<ArgType> arg,
    detail::resolver_type<MoreArgTypes>... more_args) const {
    using namespace detail;

    if constexpr (is_virtual<ArgType>::value) {
        const word* mptr;

        if constexpr (is_virtual_ptr<ArgType>) {
            mptr = arg.method_table();
        } else {
            mptr = get_mptr<ArgType>(arg);
        }

        auto slot = slots_strides[0];

        // The first virtual parameter is special.  Since its stride is
        // 1, there is no need to store it. Also, the method table
        // contains a pointer into the multi-dimensional dispatch table,
        // already resolved to the appropriate group.
        auto dispatch = mptr[slot].pw;
        return resolve_multi_next<1, MoreArgTypes...>(dispatch, more_args...);
    } else {
        return resolve_multi_first<MoreArgTypes...>(more_args...);
    }
}

template<typename Key, typename R, typename... A, class Policy>
template<size_t VirtualArg, typename ArgType, typename... MoreArgTypes>
inline void* method<Key, R(A...), Policy>::resolve_multi_next(
    const detail::word* dispatch, detail::resolver_type<ArgType> arg,
    detail::resolver_type<MoreArgTypes>... more_args) const {

    using namespace detail;

    if constexpr (is_virtual<ArgType>::value) {
        const word* mptr;

        if constexpr (is_virtual_ptr<ArgType>) {
            mptr = arg.method_table();
        } else {
            mptr = get_mptr<ArgType>(arg);
        }

        auto slot = this->slots_strides[2 * VirtualArg - 1];
        auto stride = this->slots_strides[2 * VirtualArg];
        dispatch = dispatch + mptr[slot].i * stride;
    }

    if constexpr (VirtualArg + 1 == arity) {
        return dispatch->pf;
    } else {
        return resolve_multi_next<VirtualArg + 1, MoreArgTypes...>(
            dispatch, more_args...);
    }
}

template<typename Key, typename R, typename... A, class Policy>
typename method<Key, R(A...), Policy>::return_type
method<Key, R(A...), Policy>::not_implemented_handler(
    detail::remove_virtual<A>... args) {
    resolution_error error;
    error.status = resolution_error::no_definition;
    error.method = &typeid(method);
    error.arity = arity;
    detail::ti_ptr tis[sizeof...(args)];
    error.tis = tis;
    auto ti_iter = tis;
    (..., (*ti_iter++ = detail::get_tip<A>(args)));
    detail::error_handler(error_type(std::move(error)));
    abort(); // in case user handler "forgets" to abort
}

template<typename Key, typename R, typename... A, class Policy>
typename method<Key, R(A...), Policy>::return_type
method<Key, R(A...), Policy>::ambiguous_handler(
    detail::remove_virtual<A>... args) {
    resolution_error error;
    error.status = resolution_error::ambiguous;
    error.method = &typeid(method);
    error.arity = arity;
    detail::ti_ptr tis[sizeof...(args)];
    error.tis = tis;
    auto ti_iter = tis;
    (..., (*ti_iter++ = detail::get_tip<A>(args)));
    detail::error_handler(error_type(std::move(error)));
    abort(); // in case user handler "forgets" to abort
}

template<class Class, class Policy, typename Box>
template<class Other>
inline auto
virtual_ptr_aux<Class, Policy, Box>::dynamic_method_table(Other& obj) {
    using namespace detail;

    static_assert(
        std::is_polymorphic_v<
            typename virtual_traits<Other&>::polymorphic_type>,
        "use 'final' if intended");

    mptr_type mptr;

    auto dynamic_key = virtual_traits<Other&>::key(obj);
    auto static_key =
        &typeid(typename virtual_traits<Other&>::polymorphic_type);

    if (dynamic_key == static_key) {
        if constexpr (Policy::use_indirect_method_pointers) {
            mptr = Policy::template indirect_method_table<
                typename detail::virtual_traits<Other&>::polymorphic_type>;
        } else {
            mptr = Policy::template method_table<
                typename detail::virtual_traits<Other&>::polymorphic_type>;
        }
    } else {
        auto index = Policy::context.hash(dynamic_key);

        if constexpr (Policy::use_indirect_method_pointers) {
            mptr = Policy::context.indirect_mptrs[index];
        } else {
            mptr = Policy::context.mptrs[index];
        }
    }

    return mptr;
}

namespace policy {

struct yOMM2_API runtime_checks_mixin : virtual abstract_policy {
    static constexpr bool runtime_checks = true;
};

template<typename Stream>
struct runtime_trace_mixin : virtual abstract_policy {
    static Stream trace;
};

template<typename Stream>
inline Stream runtime_trace_mixin<Stream>::trace;

template<class Policy>
struct scope_mixin : virtual abstract_policy {
    static struct context context;
    static struct catalog catalog;
};

template<class Policy>
struct method_tables_mixin : virtual abstract_policy {
    template<class Class>
    static detail::mptr_type method_table;
    template<class Class>
    static detail::mptr_type* indirect_method_table;
};

template<class Policy>
template<class Class>
detail::mptr_type method_tables_mixin<Policy>::method_table;

template<class Policy>
template<class Class>
detail::mptr_type* method_tables_mixin<Policy>::indirect_method_table =
    &method_tables_mixin<Policy>::method_table<Class>;

struct basic_policy : scope_mixin<basic_policy>,
                      method_tables_mixin<basic_policy> {};

struct debug : basic_policy,
               runtime_checks_mixin,
               runtime_trace_mixin<detail::stdostream> {};
struct release : basic_policy {};

struct yOMM2_API abstract_shared : abstract_policy {
    static constexpr bool enable_runtime_checks = true;
    static constexpr bool use_indirect_method_pointers = false;
    static detail::stdostream trace;
    static struct context context;
    static struct catalog catalog;
    template<class Class>
    static detail::mptr_type method_table;
    template<class Class>
    static detail::mptr_type* indirect_method_table;
};

struct debug_shared : abstract_shared {
    static constexpr bool runtime_checks = true;
};

struct release_shared : abstract_shared {
    static constexpr bool runtime_checks = false;
};

template<class Class>
detail::mptr_type abstract_shared::method_table;

template<class Class>
detail::mptr_type* abstract_shared::indirect_method_table =
    &abstract_shared::method_table<Class>;

template<class Policy>
catalog scope_mixin<Policy>::catalog;

template<class Policy>
context scope_mixin<Policy>::context;

} // namespace policy

inline error_handler_type set_error_handler(error_handler_type handler) {
    auto prev = detail::error_handler;
    detail::error_handler = handler;
    return prev;
}

inline method_call_error_handler yOMM2_API
set_method_call_error_handler(method_call_error_handler handler) {
    auto prev = detail::method_call_error_handler_p;
    detail::method_call_error_handler_p = handler;
    return prev;
}

template<class Policy>
void update();

#ifdef YOMM2_SHARED
yOMM2_API void update();
#else
inline void update() {
    update<default_policy>();
}
#endif

[[deprecated("use update() instead")]] yOMM2_API inline void update_methods() {
    update();
}

namespace detail {

inline void default_method_call_error_handler(
    const method_call_error& error, size_t arity, const ti_ptr ti_ptrs[]) {
    if constexpr (bool(debug)) {
        const char* explanation[] = {
            "no applicable definition", "ambiguous call"};
        detail::cerr
            << explanation[error.code - resolution_error::no_definition]
            << " for " << error.method_name << "(";
        auto comma = "";
        for (auto ti : range{ti_ptrs, ti_ptrs + arity}) {
            detail::cerr << comma << ti->name();
            comma = ", ";
        }
        detail::cerr << ")\n";
    }
    abort();
}

inline void default_error_handler(const error_type& error_v) {
    if (auto error = std::get_if<resolution_error>(&error_v)) {
        method_call_error old_error;
        old_error.code = error->status;
        old_error.method_name = error->method->name();
        method_call_error_handler_p(
            std::move(old_error), error->arity, error->tis);
        abort();
    }

    if (auto error = std::get_if<unknown_class_error>(&error_v)) {
        if constexpr (bool(debug)) {
            detail::cerr << "unknown class " << error->ti->name() << "\n";
        }
        abort();
    }

    if (auto error = std::get_if<method_table_error>(&error_v)) {
        if constexpr (bool(debug)) {
            detail::cerr << "invalid method table for " << error->ti->name()
                         << "\n";
        }
        abort();
    }

    if (auto error = std::get_if<hash_search_error>(&error_v)) {
        if constexpr (bool(debug)) {
            detail::cerr << "could not find hash factors after "
                         << error->attempts << " in " << error->duration.count()
                         << "s using " << error->buckets << " buckets\n";
        }
        abort();
    }
}

} // namespace detail

} // namespace yomm2
} // namespace yorel

#ifndef YOMM2_SHARED
    #include <yorel/yomm2/runtime.hpp>
#endif

#endif
