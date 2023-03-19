// Copyright (c) 2021 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <type_traits>

struct base {
    virtual ~base() {
    }
};

struct derived : base {};
struct v_derived : virtual base {};

template<typename B, typename D, typename = void>
struct cast {
    static D& fn(B& obj) {
        std::cout << "dynamic_cast\n";
        return dynamic_cast<D&>(obj);
    }
};

template<typename B, typename D>
struct cast<B, D, std::void_t<decltype(static_cast<D*>((B*)nullptr))>> {
    static D& fn(B& obj) {
        std::cout << "static_cast\n";
        return static_cast<D&>(obj);
    }
};

template<typename T>
void test(const base& b) {
    cast<const base, const T>::fn(b);
}

int main() {
    derived d;
    test<derived>(d);
    v_derived v;
    test<v_derived>(v);
}