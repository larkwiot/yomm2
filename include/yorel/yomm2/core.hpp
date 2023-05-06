#ifndef YOREL_YOMM2_CORE_INCLUDED
#define YOREL_YOMM2_CORE_INCLUDED

#include <chrono>
#include <memory>
#include <string_view>
#include <variant>
#include <vector>

#if defined(YOMM2_ENABLE_TRACE) && (YOMM2_ENABLE_TRACE & 2)
    #include <iostream>
#else
    #include <iosfwd>
#endif

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

namespace yorel {
namespace yomm2 {

namespace detail {

union word {
    void* pf;
    const word* pw;
    size_t i;
    const void* ti;
};

using ti_ptr = const std::type_info*;

struct hash_function {
    std::uintptr_t mult;
    std::size_t shift;

    std::size_t operator()(const void* p) const {
        return static_cast<std::size_t>(
            (mult * reinterpret_cast<std::uintptr_t>(const_cast<void*>(p))) >>
            shift);
    }
};

} // namespace detail

template<typename T>
struct virtual_;

template<typename... Types>
struct types;

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

yOMM2_API error_handler_type set_error_handler(error_handler_type handler);

struct catalog;

struct context {
    std::vector<detail::word> gv;
    std::vector<detail::word*> mptrs;
    std::vector<detail::ti_ptr> control;
    std::vector<detail::word**> indirect_mptrs;
    detail::hash_function hash;
};

namespace policy {

struct abstract_policy;
struct hash_factors_in_globals;

} // namespace policy

using default_policy = policy::hash_factors_in_globals;

template<typename Class, typename... Rest>
struct class_declaration;

struct direct;
struct indirect;

template<class Class, class Policy = default_policy>
class virtual_ptr;

} // namespace yomm2
} // namespace yorel

#include <yorel/yomm2/detail.hpp>

namespace yorel {
namespace yomm2 {

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

yOMM2_API method_call_error_handler
set_method_call_error_handler(method_call_error_handler handler);

// end deprecated

template<typename T>
struct virtual_;

template<typename... Types>
struct types;

struct catalog {
    catalog& add(detail::class_info& cls) {
        classes.push_front(cls);
        return *this;
    }

    static_chain<detail::class_info> classes;
    static_chain<detail::method_info> methods;
};

namespace policy {

struct abstract_policy {
    static constexpr bool use_indirect_method_pointers = false;
#ifdef NDEBUG
    static constexpr bool enable_runtime_checks = false;
#else
    static constexpr bool enable_runtime_checks = true;
#endif
    ;
};

struct yOMM2_API global_context : virtual abstract_policy {
    static struct context context;
};

struct yOMM2_API global_catalog : virtual abstract_policy {
    static struct catalog catalog;
};

struct hash_factors_in_globals : global_catalog, global_context {
    using method_info_type = detail::method_info;

    template<typename>
    static auto hash() {
        return context.hash;
    }
};

struct hash_factors_in_method : global_catalog, global_context {
    struct yOMM2_API method_info_type : detail::method_info {
        detail::hash_function hash;
        void install_hash_factors(detail::runtime& rt) override;
    };

    template<typename Method>
    static auto hash() {
        return Method::fn.hash;
    }
};

} // namespace policy

template<typename Key, typename Signature, class Policy = default_policy>
struct method;

template<typename Key, typename R, typename... A, class Policy>
struct method<Key, R(A...), Policy> : Policy::method_info_type {
    using self_type = method;
    using policy_type = Policy;
    using declared_argument_types = types<A...>;
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
    // For multi-methods: the offset of the first virtual argument in the method
    // table, which contains a pointer to the corresponding cell in the dispatch
    // table, followed by the offset of the second argument and the stride in
    // the second dimension, etc.

    static method fn;
    static function_pointer_type fake_definition;

    explicit method(std::string_view name = typeid(method).name()) {
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

    method(const method&) = delete;
    method(method&&) = delete;

    ~method() {
        Policy::catalog.methods.remove(*this);
    }

    template<typename ArgType, typename... MoreArgTypes>
    void* resolve_uni(
        detail::resolver_type<ArgType> arg,
        detail::resolver_type<MoreArgTypes>... more_args) const {

        using namespace detail;

        if constexpr (is_virtual<ArgType>::value) {
            const word* mptr = get_mptr<method, ArgType>(arg);
            call_trace << " slot = " << this->slots_strides[0];
            return mptr[this->slots_strides[0]].pf;
        } else {
            return resolve_uni<MoreArgTypes...>(more_args...);
        }
    }

    template<size_t VirtualArg, typename ArgType, typename... MoreArgTypes>
    void* resolve_multi_first(
        detail::resolver_type<ArgType> arg,
        detail::resolver_type<MoreArgTypes>... more_args) const {
        using namespace detail;

        if constexpr (is_virtual<ArgType>::value) {
            const word* mptr;

            if constexpr (is_virtual_ptr<ArgType>) {
                mptr = arg.method_table();
            } else {
                mptr = get_mptr<method, ArgType>(arg);
            }

            auto slot = slots_strides[0];

            if constexpr (bool(trace_enabled & TRACE_CALLS)) {
                call_trace << " slot = " << slot;
            }

            // The first virtual parameter is special.  Since its stride is 1,
            // there is no need to store it. Also, the method table contains a
            // pointer into the multi-dimensional dispatch table, already
            // resolved to the appropriate group.
            auto dispatch = mptr[slot].pw;

            if constexpr (bool(trace_enabled & TRACE_CALLS)) {
                call_trace << " dispatch = " << dispatch << "\n    ";
            }

            return resolve_multi_next<1, MoreArgTypes...>(
                dispatch, more_args...);
        } else {
            return resolve_multi_first<MoreArgTypes...>(more_args...);
        }
    }

    template<size_t VirtualArg, typename ArgType, typename... MoreArgTypes>
    void* resolve_multi_next(
        const detail::word* dispatch, detail::resolver_type<ArgType> arg,
        detail::resolver_type<MoreArgTypes>... more_args) const {

        using namespace detail;

        if constexpr (is_virtual<ArgType>::value) {
            const word* mptr;

            if constexpr (is_virtual_ptr<ArgType>) {
                mptr = arg.method_table();
            } else {
                mptr = get_mptr<method, ArgType>(arg);
            }

            auto slot = this->slots_strides[2 * VirtualArg - 1];

            if constexpr (bool(trace_enabled & TRACE_CALLS)) {
                call_trace << " slot = " << slot;
            }

            auto stride = this->slots_strides[2 * VirtualArg];

            if constexpr (bool(trace_enabled & TRACE_CALLS)) {
                call_trace << " stride = " << stride;
            }

            dispatch = dispatch + mptr[slot].i * stride;

            if constexpr (bool(trace_enabled & TRACE_CALLS)) {
                call_trace << " dispatch = " << dispatch << "\n    ";
            }
        }

        if constexpr (VirtualArg + 1 == arity) {
            return dispatch->pf;
        } else {
            return resolve_multi_next<VirtualArg + 1, MoreArgTypes...>(
                dispatch, more_args...);
        }
    }

    template<typename... ArgType>
    auto resolve(detail::resolver_type<ArgType>... args) const {
        using namespace detail;

        if constexpr (bool(trace_enabled & TRACE_CALLS)) {
            call_trace << "call " << this->name << "\n";
        }

        void* pf;

        if constexpr (arity == 1) {
            pf = resolve_uni<ArgType...>(args...);
        } else {
            pf = resolve_multi_first<0, ArgType...>(args...);
        }

        call_trace << " pf = " << pf << "\n";

        return reinterpret_cast<function_pointer_type>(pf);
    }

    return_type operator()(detail::remove_virtual<A>... args) const {
        using namespace detail;
        return resolve<A...>(argument_traits<A>::rarg(args)...)(
            std::forward<remove_virtual<A>>(args)...);
    }

    static return_type
    not_implemented_handler(detail::remove_virtual<A>... args) {
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

    static return_type ambiguous_handler(detail::remove_virtual<A>... args) {
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

// clang-format off

template<typename Class, typename... Rest>
struct class_declaration : class_declaration<
    detail::remove_policy<Class, Rest...>,
    detail::get_policy<Class, Rest...>
> {};

template<typename Class, typename... Bases, typename Policy>
struct class_declaration<types<Class, Bases...>, Policy> : detail::class_info {
    // Add a class to a catalog.
    // There is a possibility that the same class is registered with
    // different bases. This will be caught by augment_classes.

    using class_type = Class;
    using bases_type = types<Bases...>;

    class_declaration() {
        using namespace detail;

        ti = &typeid(class_type);
        first_base = type_id_list<bases_type>::begin;
        last_base = type_id_list<bases_type>::end;
        Policy::catalog.classes.push_front(*this);
        is_abstract = std::is_abstract_v<class_type>;
        indirect_method_table<class_type, Policy> = &method_table<class_type, Policy>;
        intrusive_mptr = &method_table<class_type, Policy>;
    }

    ~class_declaration() {
        Policy::catalog.classes.remove(*this);
    }
};

template<typename Class, typename... Bases>
struct class_declaration<types<Class, Bases...>> : class_declaration<
    types<Class, Bases...>, default_policy
> {};

// clang-format on

template<typename... T>
using use_classes = typename detail::use_classes_aux<T...>::type;

// =============================================================================
// virtual_ptr

template<class Class, class Policy>
class virtual_ptr {
    template<typename, typename, class>
    friend struct method;

    template<class OtherClass, class OtherPolicy>
    friend class virtual_ptr;

  public:
    static constexpr bool is_indirect = Policy::use_indirect_method_pointers;
    using object_type = Class;

    virtual_ptr() = delete;

    template<
        class OtherClass,
        typename = std::enable_if_t<!detail::is_virtual_ptr<OtherClass>, void>>
    virtual_ptr(OtherClass& obj) : obj(obj) {
        if constexpr (is_indirect) {
            if (typeid(obj) == typeid(OtherClass)) {
                mptr = detail::indirect_method_table<OtherClass, Policy>;
            } else {
                mptr = Policy::context
                           .indirect_mptrs[Policy::context.hash(&typeid(obj))];
            }
        } else {
            if (typeid(obj) == typeid(OtherClass)) {
                mptr = detail::method_table<OtherClass, Policy>;
            } else {
                mptr =
                    Policy::context.mptrs[Policy::context.hash(&typeid(obj))];
            }
        }
    }

    // TODO: make it work across different policies
    template<class OtherClass>
    virtual_ptr(const virtual_ptr<OtherClass, Policy>& other)
        : obj(other.obj), mptr(other.mptr) {
    }

    template<class OtherClass>
    static virtual_ptr final(OtherClass& obj) {
        using namespace detail;

        if constexpr (debug) {
            if (typeid(obj) != typeid(OtherClass)) {
                error_handler(method_table_error{&typeid(obj)});
            }
        }

        if constexpr (is_indirect) {
            return virtual_ptr(
                obj, detail::indirect_method_table<Class, Policy>);
        } else {
            return virtual_ptr(obj, detail::method_table<Class, Policy>);
        }
    }

    // TODO: make it work across different policies
    template<
        class OtherVirtualPtr,
        typename =
            std::enable_if_t<detail::is_virtual_ptr<OtherVirtualPtr>, void>>
    OtherVirtualPtr cast() const {
        return OtherVirtualPtr(
            detail::optimal_cast<typename OtherVirtualPtr::object_type&>(obj),
            mptr);
    }

    auto operator->() const {
        return obj;
    }

    auto& object() const {
        return obj;
    }

    auto& operator*() const {
        return obj;
    }

    auto unbox() const {
        return obj;
    }

    // for tests only
    auto method_table() const {
        if constexpr (is_indirect) {
            return *mptr;
        } else {
            return mptr;
        }
    }

  private:
    using mptr_type =
        std::conditional_t<is_indirect, detail::mptr_type*, detail::mptr_type>;

    using traits = detail::virtual_traits<virtual_ptr<Class>>;

    virtual_ptr(Class& obj, mptr_type mptr) : obj(obj), mptr(mptr) {
    }

    typename traits::content_type obj;
    mptr_type mptr;
};

// deduction guides
template<class Class>
virtual_ptr(Class) -> virtual_ptr<Class>;

// =============================================================================
// update_methods

yOMM2_API void update_methods();

} // namespace yomm2
} // namespace yorel

#endif
