// Copyright (c) 2018-2022 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <iostream>
#include <random>
#include <utility>

#include <benchmark/benchmark.h>
#include <yorel/yomm2/keywords.hpp>

using namespace yorel::yomm2;

template<int N, template<typename> typename Inheritance>
struct dense_matrix;

template<int N, template<typename> typename Inheritance>
struct diagonal_matrix;

template<class Base>
struct ordinary_base : Base {};

template<class Base>
struct virtual_base : virtual Base {};

template<int N, template<typename> typename Inheritance>
struct matrix {
    virtual ~matrix() {
    }
    virtual void times(int) const = 0;
    virtual void times(const matrix& other) const = 0;
    virtual void times2(const dense_matrix<N, Inheritance>& other) const = 0;
    virtual void times2(const diagonal_matrix<N, Inheritance>& other) const = 0;
};

template<int N, template<typename> typename Inheritance>
struct dense_matrix : Inheritance<matrix<N, Inheritance>> {
    virtual void times(int) const {
    }
    virtual void times(const matrix<N, Inheritance>& other) const {
        other.times2(*this);
    }
    virtual void times2(const dense_matrix<N, Inheritance>& other) const {
    }
    virtual void times2(const diagonal_matrix<N, Inheritance>& other) const {
    }
};

template<int N, template<typename> typename Inheritance>
struct diagonal_matrix : Inheritance<matrix<N, Inheritance>> {
    virtual void times(int) const {
    }
    virtual void times(const matrix<N, Inheritance>& other) const {
        other.times2(*this);
    }
    virtual void times2(const dense_matrix<N, Inheritance>& other) const {
    }
    virtual void times2(const diagonal_matrix<N, Inheritance>& other) const {
    }
};

enum { RN = 1024 * 1024 };
size_t random_numbers[RN];
size_t random_mark;

using ptr_type = void (*)();

struct Calls {
    ptr_type baseline, virtual_function, uni_method;

    template<class C>
    static void collect(Calls* iter) {
        std::cout << typeid(C).name() << "\n";
        *iter = Calls{C::baseline, C::virtual_function, C::uni_method};
    }
};

enum { NH = 5 };
Calls calls[NH];

template<template<typename> typename Inheritance, size_t... ints>
auto classes_aux(std::index_sequence<ints...> int_seq) -> types<
    matrix<ints, Inheritance>..., dense_matrix<ints, Inheritance>...,
    diagonal_matrix<ints, Inheritance>...>;

use_classes<decltype(classes_aux<ordinary_base>(
    std::make_index_sequence<NH>{}))>
    YOMM2_GENSYM;

struct YOMM2_SYMBOL(times);
template<typename M>
using times = method<YOMM2_SYMBOL(times), void(virtual_<M&>)>;

template<typename T>
void definition(T&) {}

template<template<typename> typename Inheritance, size_t... ints>
auto uni_definitions_aux(std::index_sequence<ints...> int_seq) -> std::tuple<
    typename times<matrix<ints, Inheritance>>::template add_function<definition<dense_matrix<ints, Inheritance>>>...,
    typename times<matrix<ints, Inheritance>>::template add_function<definition<diagonal_matrix<ints, Inheritance>>>...
>;

decltype(uni_definitions_aux<ordinary_base>(std::make_index_sequence<NH>{})) YOMM2_GENSYM;

template<int N, template<typename> typename Inheritance>
struct homogeneous_catalog {
    enum { OBJECTS = 100 };
    dense_matrix<N, Inheritance> dense[OBJECTS / 2];
    diagonal_matrix<N, Inheritance> diagonal[OBJECTS / 2];
    matrix<N, Inheritance>* objects[OBJECTS];

    std::default_random_engine rnd;
    std::uniform_int_distribution<std::size_t> dist{0, OBJECTS - 1};

    homogeneous_catalog() {
        std::transform(dense, dense + OBJECTS / 2, objects, [](auto& obj) {
            return &obj;
        });
        std::transform(
            diagonal, diagonal + OBJECTS / 2, objects + OBJECTS / 2,
            [](auto& obj) { return &obj; });
    }

    static homogeneous_catalog instance;

    auto draw() {
        return objects[dist(rnd)];
    }

    static void baseline() {
        auto a = instance.draw();
        auto b = instance.draw();
    }

    static void virtual_function() {
        instance.draw()->times(1);
    }

    static void uni_method() {
        times<matrix<N, Inheritance>>::fn(*instance.draw());
    }
};

template<int N, template<typename> typename Inheritance>
homogeneous_catalog<N, Inheritance>
    homogeneous_catalog<N, Inheritance>::instance;

template<template<typename> typename Inheritance, typename T, T... ints>
void init_calls(std::integer_sequence<T, ints...> int_seq) {
    auto iter = calls;
    (Calls::collect<homogeneous_catalog<ints, Inheritance>>(iter++), ...);
}

void baseline(benchmark::State& state) {
    using c = homogeneous_catalog<0, ordinary_base>;
    for (auto _ : state) {
        c::baseline();
    }
}

template<auto Field>
void run(benchmark::State& state) {
    std::default_random_engine rnd;
    std::uniform_int_distribution<std::size_t> dist{0, NH - 1};
    using c = homogeneous_catalog<0, ordinary_base>;
    for (auto _ : state) {
        (calls[dist(rnd)].*Field)();
    }
}

int main(int argc, char** argv) {
    yorel::yomm2::update_methods();

    init_calls<ordinary_base>(std::make_index_sequence<NH>{});

#define RUN(ID) benchmark::RegisterBenchmark(#ID, run<&Calls::ID>)
    RUN(baseline);
    RUN(virtual_function);
    RUN(uni_method);

    // //
    // ------------------------------------------------------------------------

    // benchmark::RegisterBenchmark(
    //     "double dispatch", default_ns::double_dispatch);
    // benchmark::RegisterBenchmark("multi-method call",
    // default_ns::multi_method);

    // //
    // ------------------------------------------------------------------------

    // benchmark::RegisterBenchmark(
    //     "virtual function call", virtual_ns::virtual_function);
    // benchmark::RegisterBenchmark(
    //     "uni-method call (virtual inheritance)", virtual_ns::times);

    // //
    // ------------------------------------------------------------------------

    // benchmark::RegisterBenchmark(
    //     "double dispatch(virtual inheritance)", virtual_ns::double_dispatch);
    // benchmark::RegisterBenchmark(
    //     "multi-method call (virtual inheritance)", virtual_ns::multi_method);

    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();

    return 0;
}
