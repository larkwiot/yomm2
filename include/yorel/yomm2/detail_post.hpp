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

inline method_call_error_handler method_call_error_handler_p =
    default_method_call_error_handler;

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

inline error_handler_type error_handler = detail::default_error_handler;

} // namespace detail
} // namespace yomm2
} // namespace yorel