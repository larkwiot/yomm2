namespace yorel {
namespace yomm2 {
namespace detail {

inline void default_method_call_error_handler(
    const method_call_error& error, size_t arity, const ti_ptr ti_ptrs[]) {
    if constexpr (bool(debug)) {
        const char* explanation[] = {
            "no applicable definition", "ambiguous call"};
        std::cerr << explanation[error.code - resolution_error::no_definition]
                  << " for " << error.method_name << "(";
        auto comma = "";
        for (auto ti : range{ti_ptrs, ti_ptrs + arity}) {
            std::cerr << comma << ti->name();
            comma = ", ";
        }
        std::cerr << ")\n" << std::flush;
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
            std::cerr << "unknown class " << error->ti->name() << "\n";
        }
        abort();
    }

    if (auto error = std::get_if<method_table_error>(&error_v)) {
        if constexpr (bool(debug)) {
            std::cerr << "invalid method table for " << error->ti->name()
                      << "\n";
        }
        abort();
    }

    if (auto error = std::get_if<hash_search_error>(&error_v)) {
        if constexpr (bool(debug)) {
            std::cerr << "could not find hash factors after " << error->attempts
                      << " in " << error->duration.count() << "s using "
                      << error->buckets << " buckets\n";
        }
        abort();
    }
}

inline auto hash_function::operator()(ti_ptr tip) const {
    auto index = unchecked_hash(tip);
    const void* test = &control;

    if constexpr (debug) {
        auto control_tip = control[index];

        if (control_tip != tip) {
            error_handler(
                unknown_class_error{unknown_class_error::call, tip});
        }
    }

    return index;
}

template<typename Method, typename ArgType>
inline auto get_mptr(resolver_type<ArgType> arg) {
    const word* mptr;
    using policy = typename Method::policy_type;

    if constexpr (has_mptr<resolver_type<ArgType>>) {
        mptr = arg.yomm2_mptr();
        check_method_pointer<policy>(mptr, virtual_traits<ArgType>::key(arg));
    } else if constexpr (is_virtual_ptr<ArgType>) {
        mptr = arg.method_table();
        // No need to check the method pointer: this was done when the
        // virtual_ptr was created.
    } else {
        auto key = virtual_traits<ArgType>::key(arg);

        if constexpr (bool(trace_enabled & TRACE_CALLS)) {
            call_trace << "  key = " << key;
        }

        mptr = policy::context.mptrs[policy::context.hash(key)];
    }

    if constexpr (bool(trace_enabled & TRACE_CALLS)) {
        call_trace << " mptr = " << mptr;
    }

    return mptr;
}

template<class Policy>
inline auto check_method_pointer(const word* mptr, ti_ptr key) {
    if constexpr (Policy::enable_runtime_checks) {
        auto& ctx = Policy::context;
        auto p = reinterpret_cast<const char*>(mptr);

        if (p == 0 && ctx.gv.empty()) {
            // no declared methods
            return mptr;
        }

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

} // namespace detail
} // namespace yomm2
} // namespace yorel