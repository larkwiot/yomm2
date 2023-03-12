// Copyright (c) 2021 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <type_traits>

void has_foo_helper(...);

template<typename Object>
auto has_foo_helper(Object* obj) -> decltype(obj->foo);

template<typename Object>
constexpr bool has_foo =
    std::is_same_v<decltype(has_foo_helper(std::declval<Object*>())), int>;

struct Foo {
    int foo;
};

struct Bar {};

static_assert(has_foo<Foo>);
static_assert(!has_foo<Bar>);

template<bool>
struct require_foo_impl;

template<>
struct require_foo_impl<false> {
    template<typename Class>
    struct crtp {
        int foo;
    };
};

template<>
struct require_foo_impl<true> {
    template<typename Class>
    struct crtp {};
};

template<typename Class>
using require_foo =
    typename require_foo_impl<has_foo<Class>>::template crtp<Class>;

struct Baz : require_foo<Baz> {
};

struct Foolish : require_foo<Foolish> {
    int foo;
};

//static_assert(has_foo<Foolish>);
static_assert(!has_foo<Baz>);

int main() {
    Baz baz;
    baz.foo;
    std::cout << sizeof(baz) << "\n";

    Foolish foolish;
    foolish.foo;
    std::cout << sizeof(foolish) << "\n";
}