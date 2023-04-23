// Copyright (c) 2021 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#define YOMM2_ENABLE_TRACE 3

#include <yorel/yomm2/keywords.hpp>
#include <yorel/yomm2/templates.hpp>

#define BOOST_TEST_MODULE yomm2
#include <boost/test/included/unit_test.hpp>
#include <boost/utility/identity_type.hpp>

using namespace yorel::yomm2;

namespace yorel {
namespace yomm2 {
struct direct {};
struct indirect {};
} // namespace yomm2
} // namespace yorel

using indirection_types = types<direct, indirect>;

template<typename>
struct test_policy_ : policy::default_policy {
    static struct catalog catalog;
    static struct context context;
};

template<typename T>
catalog test_policy_<T>::catalog;
template<typename T>
context test_policy_<T>::context;

struct Animal {
    virtual ~Animal() {
    }
};

struct Cat : Animal {};

namespace direct_virtual_ptr {

struct key;
using test_policy = test_policy_<key>;

Cat cat;
virtual_ptr<Cat> v_cat(cat);
virtual_ptr<Animal> v_animal(v_cat);

register_classes(test_policy, Animal, Cat);

BOOST_AUTO_TEST_CASE_TEMPLATE(
    test_virtual_ptr, Indirection, indirection_types) {
    using namespace detail;

    detail::update_methods(test_policy::catalog, test_policy::context);

    using vptr_animal = virtual_ptr<Animal, Indirection, test_policy>;
    static_assert(detail::is_virtual_ptr<vptr_animal>);
    using vptr_cat = virtual_ptr<Cat, Indirection, test_policy>;

    Animal animal;
    auto virtual_animal = vptr_animal::final(animal);
    BOOST_TEST(&virtual_animal.object() == &animal);
    BOOST_TEST(
        (virtual_animal.method_table() == method_table<Animal, test_policy>));

    Cat cat;
    BOOST_TEST((&vptr_cat::final(cat).object()) == &cat);
    BOOST_TEST((
        vptr_cat::final(cat).method_table() == method_table<Cat, test_policy>));

    BOOST_TEST(
        (vptr_animal(cat).method_table() == method_table<Cat, test_policy>));

    vptr_animal virtual_cat_ptr(cat);
    static method<Animal, std::string(virtual_<Animal&>), test_policy>
        YOMM2_GENSYM;
    detail::update_methods(test_policy::catalog, test_policy::context);
    BOOST_TEST(
        (virtual_cat_ptr.method_table() == method_table<Cat, test_policy>) ==
        vptr_animal::is_indirect);
}
} // namespace direct_virtual_ptr

#ifndef NDEBUG

namespace bad_virtual_ptr {

struct Animal {
    virtual ~Animal() {
    }
};

using test_policy = test_policy_<Animal>;

struct Dog : Animal {};

register_classes(Animal, Dog, test_policy);

declare_method(std::string, kick, (virtual_<Animal&>), test_policy);

BOOST_AUTO_TEST_CASE(test_bad_virtual_ptr) {
    auto prev_handler = set_error_handler([](const error_type& ev) {
        if (auto error = std::get_if<method_table_error>(&ev)) {
            static_assert(
                std::is_same_v<decltype(error), const method_table_error*>);
            throw *error;
        }
    });

    update_methods();

    try {
        Dog snoopy;
        Animal& animal = snoopy;
        virtual_ptr<Animal, direct, test_policy>::final(animal);
    } catch (const method_table_error& error) {
        BOOST_TEST(error.ti->name() == typeid(Dog).name());
        return;
    } catch (...) {
        BOOST_FAIL("wrong exception");
    }

    BOOST_FAIL("did not throw");

    set_error_handler(prev_handler);
}
} // namespace bad_virtual_ptr

#endif

namespace test_virtual_ptr_dispatch {

struct Character {
    virtual ~Character() {
    }
};

struct Warrior : Character {};

struct Bear : Character {};

struct Object {
    virtual ~Object() {
    }
};

struct Axe : Object {};

use_classes<Character, Warrior, Object, Axe, Bear> YOMM2_GENSYM;

using kick = method<void, std::string(virtual_ptr<Character>)>;

static auto definition(virtual_ptr<Bear>) {
    return std::string("growl");
}

kick::add_function<definition> YOMM2_GENSYM;

BOOST_AUTO_TEST_CASE_TEMPLATE(
    test_virtual_ptr_dispatch_uni_method, IndirectionType, indirection_types) {
    // detail::trace_flags = 3;
    // detail::logs = &std::cerr;
    update_methods();

    Bear bear;
    BOOST_TEST(kick::fn(virtual_ptr<Character>(bear)) == "growl");
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    test_virtual_ptr_dispatch_multi_method, IndirectionType, indirection_types) {
    // detail::trace_flags = 3;
    // detail::logs = &std::cerr;

    using fight = method<
        void,
        std::string(
            virtual_ptr<Character, IndirectionType>,
            virtual_ptr<Object, IndirectionType>,
            virtual_ptr<Character, IndirectionType>)>;

    struct definition {
        static std::string
        fn(virtual_ptr<Warrior, IndirectionType>,
           virtual_ptr<Axe, IndirectionType>,
           virtual_ptr<Bear, IndirectionType>) {
            return "kill bear";
        }
    };

    static typename fight::template add_definition<definition> YOMM2_GENSYM;

    update_methods();

    Warrior warrior;
    Axe axe;
    Bear bear;
   BOOST_TEST(fight::fn(warrior, axe, bear) == "kill bear");
}

} // namespace test_virtual_ptr_dispatch