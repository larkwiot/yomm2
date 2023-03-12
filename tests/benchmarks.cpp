// Copyright (c) 2018-2022 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// clang-format off

#include <algorithm>
#include <iostream>
#include <random>
#include <sstream>
#include <utility>

#include <benchmark/benchmark.h>
#include <yorel/yomm2/keywords.hpp>
#include <yorel/yomm2/intrusive.hpp>
#include <yorel/yomm2/templates.hpp>

#include "benchmarks_parameters.hpp"

using namespace yorel::yomm2;
using namespace yorel::yomm2::policy;
using namespace boost::mp11;

namespace yomm = yorel::yomm2;

#if !defined(NDEBUG)
enum { NH = 2 };
#elif defined(YOMM2_BENCHMARK_HIERARCHIES)
enum { NH = YOMM2_BENCHMARK_HIERARCHIES };
#else
enum { NH = 100 };
#endif

const std::string yomm2_ = "yomm2_";

auto OBJECTS() {
    static size_t n = 1000;

    if (auto objects_from_env = getenv("YOMM2_BENCHMARKS_OBJECTS")) {
        n = std::stoi(objects_from_env);
    }

    return n;
}

std::default_random_engine alloc_rnd;
std::uniform_int_distribution<std::size_t> alloc_dist{0, 500};

struct YOMM2_SYMBOL(kick);
struct YOMM2_SYMBOL(meet);

template<typename N, typename Inheritance, typename Work>
struct Dog;

template<typename N, typename Inheritance, typename Work>
struct Cat;

struct vfunc {};
struct baseline {};
template<size_t N>
using int_ = std::integral_constant<size_t, N>;
using policies = types<hash_factors_in_globals, hash_factors_in_method>;

struct ordinary_base {
    static const char* name() { return "ordinary_base"; }
    template<class Base>
    struct base : Base {};
};

struct virtual_base {
    static const char* name() { return "virtual_base"; }
    template<class Base>
    struct base : virtual Base {};
};

using inheritance_types = types<ordinary_base, virtual_base>;

struct no_work {
    static const char* name() { return "no_work"; }

    using return_type = void;
    static void fn(std::uintptr_t, std::uintptr_t) {
    }
};

struct some_work {
    static const char* name() { return "some_work"; }

    static std::uintptr_t fn(std::uintptr_t a, std::uintptr_t b) {
        auto x = 1;

        for (int i = 0; i < 8; ++i) {
            if (a & 1) {
                x += 1;
            }

            if (b & 1) {
                x *= 2;
            }

            a >>= 1;
            b >>= 1;
        }

        return x;
    }

    using return_type = decltype(
        fn(std::declval<std::uintptr_t>(), std::declval<std::uintptr_t>())
    );
};

using work_types = types<no_work, some_work>;

template<typename Class>
struct empty_class {};

struct virtual_dispatch {
    static constexpr auto name() { return "virtual_function"; };

    template<typename Class>
    using base = empty_class<Class>;
    template<typename Class>
    using derived = empty_class<Class>;

    template<class population>
    struct dispatcher {
        static auto kick(typename population::Animal& object) {
            return object.kick();
        }
        static auto meet(typename population::Animal& a, typename population::Animal& b) {
            return a.meet(b);
        }
    };
};

struct method_dispatch {
    template<class Population>
    struct dispatcher {
        using Animal = typename Population::Animal;
        using Dog = typename Population::Dog;
        using Cat = typename Population::Cat;
        using Work = typename Population::work;
        using Policy = typename Population::policy;
        using work_return_type = typename Work::return_type;

        template<typename...>
        struct definition;

        using kick_method = method<
            Population,
            work_return_type(virtual_<Animal&>),
            Policy
        >;

        template<class Method, class T>
        struct definition<Method, T> {
            static auto fn(T& a) {
                return Work::fn(std::uintptr_t(&a), std::uintptr_t(&a));
            }
        };

        typename kick_method::template add_definition<definition<kick_method, Dog>> YOMM2_GENSYM;
        typename kick_method::template add_definition<definition<kick_method, Cat>> YOMM2_GENSYM;

        static auto kick(Animal& object) {
            return kick_method::fn(object);
        }

        using meet_method = method<
            Population,
            work_return_type(virtual_<Animal&>, virtual_<Animal&>),
            Policy
        >;

        template<class Method, class T, class U>
        struct definition<Method, T, U> {
            static auto fn(T& a, U& b) {
                return Work::fn(std::uintptr_t(&a), std::uintptr_t(&a));
            }
        };

        typename meet_method::template add_definition<definition<meet_method, Dog, Dog>> YOMM2_GENSYM;
        typename meet_method::template add_definition<definition<meet_method, Dog, Cat>> YOMM2_GENSYM;
        typename meet_method::template add_definition<definition<meet_method, Cat, Dog>> YOMM2_GENSYM;
        typename meet_method::template add_definition<definition<meet_method, Cat, Cat>> YOMM2_GENSYM;

        static auto meet(Animal& a, Animal& b) {
            return meet_method::fn(a, b);
        }
    };
};

template<typename Policy>
struct orthogonal_dispatch : method_dispatch {
    static constexpr const char* name();

    template<typename Class>
    using base = empty_class<Class>;
    template<typename Class>
    using derived = empty_class<Class>;
    using policy = Policy;
};

template<>
constexpr const char* orthogonal_dispatch<hash_factors_in_globals>::name() {
    return "hash_factors_in_globals";
}

template<>
constexpr const char* orthogonal_dispatch<hash_factors_in_method>::name() {
    return "hash_factors_in_method";
}

struct direct_intrusive_dispatch : method_dispatch {
    static constexpr auto name() { return "direct_intrusive"; };

    template<typename Class>
    using base = yomm::base<Class, direct>;
    template<typename Class>
    using derived = yomm::derived<Class>;
    using policy = default_policy;
};

struct indirect_intrusive_dispatch : method_dispatch {
    static constexpr auto name() { return "indirect_intrusive"; };

    template<typename Class>
    using base = yomm::base<Class, indirect>;
    template<typename Class>
    using derived = yomm::derived<Class>;
    using policy = default_policy;
};

using dispatch_types = types<
    virtual_dispatch,
    orthogonal_dispatch<hash_factors_in_globals>,
    orthogonal_dispatch<hash_factors_in_method>,
    direct_intrusive_dispatch,
    indirect_intrusive_dispatch
>;

struct abstract_population {
    virtual void* allocate() = 0;
    virtual void call_0() = 0;
    virtual void call_1() = 0;
    virtual void call_2() = 0;
};

template<typename N, typename InheritancePolicy, typename DispatchPolicy, typename Work>
struct population : abstract_population, DispatchPolicy {
    struct Dog;
    struct Cat;
    using work = Work;
    using work_return_type = typename Work::return_type;

    struct Animal : DispatchPolicy::template base<Animal> {
        virtual ~Animal() {}
        virtual work_return_type kick() = 0;
        virtual work_return_type meet(Animal& other) = 0;
        virtual work_return_type meet(Dog& other) = 0;
        virtual work_return_type meet(Cat& other) = 0;
    };

    struct Dog :
        InheritancePolicy::template base<Animal>, 
        DispatchPolicy::template derived<Dog> {
        virtual work_return_type kick() {
            return Work::fn(std::uintptr_t(this), std::uintptr_t(this));
        }

        virtual work_return_type meet(Animal& other) {
            return other.meet(*this);
        }

        virtual work_return_type meet(Dog& other) {
            return Work::fn(std::uintptr_t(this), std::uintptr_t(&other));
        }

        virtual work_return_type meet(Cat& other) {
            return Work::fn(std::uintptr_t(this), std::uintptr_t(&other));
        }
    };
    
    struct Cat :
        InheritancePolicy::template base<Animal>, 
        DispatchPolicy::template derived<Cat> {
        virtual work_return_type kick() {
            return Work::fn(std::uintptr_t(this), std::uintptr_t(this));
        }

        virtual work_return_type meet(Animal& other) {
            return other.meet(*this);
        }

        virtual work_return_type meet(Dog& other) {
            return Work::fn(std::uintptr_t(this), std::uintptr_t(&other));
        }

        virtual work_return_type meet(Cat& other) {
            return Work::fn(std::uintptr_t(this), std::uintptr_t(&other));
        }
    };

    use_classes<Animal, Dog, Cat> YOMM2_GENSYM;

    using dispatcher = typename DispatchPolicy::template dispatcher<population>;
    dispatcher YOMM2_GENSYM;

    std::default_random_engine rnd;
    std::uniform_int_distribution<std::size_t> dist{0, OBJECTS() - 1};
    std::vector<Animal*> objects;

    static population instance;

    ~population() {
        for (auto p : objects) {
            delete p;
        }
    }

    void* allocate() override {
        if (objects.size() == OBJECTS()) {
            return nullptr;
        }

        auto obj = (objects.size() & 1) ? (Animal*) new Cat : new Dog;
        objects.push_back(obj);

        return obj;
    }

    Animal& draw() {
        return *objects[dist(rnd)];
    }

    void call_0() override {
        draw();
        draw();
    }

    void call_1() override {
        draw();
        dispatcher::kick(draw());

    }

    void call_2() override {
        dispatcher::meet(draw(), draw());
    }
};

template<typename N, typename InheritancePolicy, typename DispatchPolicy, typename Work>
population<N, InheritancePolicy, DispatchPolicy, Work>
population<N, InheritancePolicy, DispatchPolicy, Work>::instance;

template<typename Inheritance, typename DispatchPolicy, typename Work>
std::vector<abstract_population*> populations;

std::vector<abstract_population*> all_populations;

template<typename Inheritance, typename Dispatch, typename Work, typename Arity>
struct Benchmark {
    static std::string name;

    Benchmark() {
        benchmark::RegisterBenchmark(name.c_str(), run);
    }

    static void run(benchmark::State& state) {
        std::default_random_engine rnd;
        std::uniform_int_distribution<std::size_t> dist{0, NH - 1};
        for (auto _ : state) {
            if constexpr (Arity::value == 0) {
                populations<Dispatch, Inheritance, Work>[dist(rnd)]->call_0();
            } else if constexpr (Arity::value == 1) {
                populations<Dispatch, Inheritance, Work>[dist(rnd)]->call_1();
            } else {
                static_assert(Arity::value == 2);
                populations<Dispatch, Inheritance, Work>[dist(rnd)]->call_2();
            }
        }
    }
};

template<typename Inheritance, typename Dispatch, typename Work, typename Arity>
std::string Benchmark<Inheritance, Dispatch, Work, Arity>::name =
    std::string(Dispatch::name())
        + "-arity_" + std::to_string(Arity::value)
        + "-" + Inheritance::name()
        + "-" + Work::name();

int main(int argc, char** argv) {
    yorel::yomm2::update_methods();

    std::ostringstream version;
#if defined(__clang__)
    auto compiler = "clang";
    version << __clang_major__ << "." << __clang_minor__ << "."
            << __clang_patchlevel__;
#else
#ifdef __GNUC__
    auto compiler = "gcc";
    version << __GNUC__ << "." << __GNUC_MINOR__;
#endif
#endif
#ifdef _MSC_VER
    auto compiler = "Visual Studio";
    version << _MSC_VER;
#endif
    benchmark::AddCustomContext("yomm2_compiler", compiler);
    benchmark::AddCustomContext("yomm2_compiler_version", version.str());
#ifdef NDEBUG
    benchmark::AddCustomContext("yomm2_build_type", "release");
#else
    benchmark::AddCustomContext("yomm2_build_type", "debug");
#endif
    benchmark::AddCustomContext("yomm2_hierarchies", std::to_string(NH));
    benchmark::AddCustomContext("yomm2_objects", std::to_string(OBJECTS()));

    std::vector<uintptr_t> vptrs;

    mp_for_each<mp_iota_c<NH>>([&vptrs](auto I_value) {
        using I = decltype(I_value);
        mp_for_each<dispatch_types>([&vptrs](auto D_value) {
            using D = decltype(D_value);
            mp_for_each<inheritance_types>([&vptrs](auto INH_value) {
                using INH = decltype(INH_value);
                mp_for_each<work_types>([&vptrs](auto W_value) {
                    using W = decltype(W_value);
                    populations<D, INH, W>.push_back(&population<I, INH, D, W>::instance);
                    all_populations.push_back(&population<I, INH, D, W>::instance);
                });
            });
        });
    });

    std::vector<uintptr_t> obj_ptrs;

    {
        std::default_random_engine rnd;
        std::uniform_int_distribution<std::size_t> dist;

        auto incomplete_populations = all_populations;

        while (!incomplete_populations.empty()) {
            // pick one at random
            auto i = dist(rnd) % incomplete_populations.size();
            auto population_set = incomplete_populations[i];
            // make it allocate one object
            if (auto obj = population_set->allocate()) {
                obj_ptrs.push_back(reinterpret_cast<uintptr_t>(obj));
            } else {
                // if it is full, remove it
                incomplete_populations.erase(incomplete_populations.begin() + i);
            }
        }
    }

    Benchmark<ordinary_base, virtual_dispatch, no_work, int_<0>> YOMM2_GENSYM;

    // clang-format off
    mp_apply<
        std::tuple,
        apply_product<
            templates<Benchmark>,
            inheritance_types,
            dispatch_types,
            types<no_work, some_work>,
            types<
                std::integral_constant<size_t, 1>,
                std::integral_constant<size_t, 2>
            >
        >
    > YOMM2_GENSYM;

    population<std::integral_constant<size_t, 0>, ordinary_base, direct_intrusive_dispatch, no_work>::instance.call_1();

    benchmark::Initialize(&argc, argv);

    if (benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }

    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();

    return 0;
}

void call_hash_factors_in_globals_1(population<std::integral_constant<size_t, 0>, ordinary_base, orthogonal_dispatch<hash_factors_in_globals>, no_work>::Animal& a) {
    population<std::integral_constant<size_t, 0>, ordinary_base, orthogonal_dispatch<hash_factors_in_globals>, no_work>::dispatcher::kick(a);
}

void call_direct_intrusive_1(population<std::integral_constant<size_t, 0>, ordinary_base, direct_intrusive_dispatch, no_work>::Animal& a) {
    population<std::integral_constant<size_t, 0>, ordinary_base, direct_intrusive_dispatch, no_work>::dispatcher::kick(a);
	// movq	  8(%rdi),                       %rax
	// movslq method.fn.slots_strides(%rip), %rcx
	// jmpq	*(%rax,%rcx,8)
}
