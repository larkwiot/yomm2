// Copyright (c) 2018-2022 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#if defined(YOMM2_SHARED)
    #if defined(_WIN32)
        #define yOMM2_API __declspec(dllexport)
    #else
        #define yOMM2_API __attribute__((__visibility__("default")))
    #endif
#else
    #error "This should be compiled only for a shared library build."
#endif

#include <yorel/yomm2/core.hpp>
#include <yorel/yomm2/runtime.hpp>

namespace yorel {
namespace yomm2 {

template yOMM2_API void update<default_policy>();

yOMM2_API void update() {
    update<default_policy>();
}

namespace detail {
yOMM2_API std::ostream* logs;
yOMM2_API unsigned trace_flags;
} // namespace detail

namespace policy {
yOMM2_API context library_policy::context;
yOMM2_API catalog library_policy::catalog;
} // namespace policy

} // namespace yomm2
} // namespace yorel
