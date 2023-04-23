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

struct Animal {
    virtual ~Animal() {
    }
};

struct Cat : Animal {};
struct Dog : Animal {};

using indirection_types = types<direct, indirect>;

template<typename Key, bool Indirect>
struct test_policy_ : policy::default_policy {
    static struct catalog catalog;
    static struct context context;
    static constexpr bool indirect_method_pointer = Indirect;
};

template<typename Key, bool Indirect>
catalog test_policy_<Key, Indirect>::catalog;
template<typename Key, bool Indirect>
context test_policy_<Key, Indirect>::context;

template<typename Key>
using policy_types = types<test_policy_<Key, false>, test_policy_<Key, true>>;

namespace test_virtual_ptr {

struct key {};

BOOST_AUTO_TEST_CASE_TEMPLATE(test_virtual_ptr, Policy, policy_types<key>) {
    using namespace detail;

    static use_classes<Policy, Animal, Cat> YOMM2_GENSYM;
    detail::update_methods(Policy::catalog, Policy::context);

    using vptr_animal = virtual_ptr<Animal, Policy>;
    static_assert(detail::is_virtual_ptr<vptr_animal>);
    using vptr_cat = virtual_ptr<Cat, Policy>;

    Animal animal;
    auto virtual_animal = vptr_animal::final(animal);
    BOOST_TEST(&virtual_animal.object() == &animal);
    BOOST_TEST((virtual_animal.method_table() == method_table<Animal, Policy>));

    Cat cat;
    BOOST_TEST((&vptr_cat::final(cat).object()) == &cat);
    BOOST_TEST(
        (vptr_cat::final(cat).method_table() == method_table<Cat, Policy>));

    BOOST_TEST((vptr_animal(cat).method_table() == method_table<Cat, Policy>));

    vptr_animal virtual_cat_ptr(cat);
    static method<Animal, std::string(virtual_<Animal&>), Policy> YOMM2_GENSYM;
    detail::update_methods(Policy::catalog, Policy::context);
    BOOST_TEST(
        (virtual_cat_ptr.method_table() == method_table<Cat, Policy>) ==
        vptr_animal::is_indirect);
}
} // namespace test_virtual_ptr

#ifndef NDEBUG

namespace bad_virtual_ptr {

struct key {};
using test_policy = test_policy_<key, false>;

register_classes(test_policy, Animal, Dog);

declare_method(std::string, kick, (virtual_<Animal&>), test_policy);

BOOST_AUTO_TEST_CASE(test_final) {
    auto prev_handler = set_error_handler([](const error_type& ev) {
        if (auto error = std::get_if<method_table_error>(&ev)) {
            static_assert(
                std::is_same_v<decltype(error), const method_table_error*>);
            throw *error;
        }
    });

    detail::update_methods(test_policy::catalog, test_policy::context);

    try {
        Dog snoopy;
        Animal& animal = snoopy;
        virtual_ptr<Animal, test_policy>::final(animal);
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

struct key {};

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

BOOST_AUTO_TEST_CASE_TEMPLATE(
    test_virtual_ptr_dispatch, Policy, policy_types<key>) {

    static use_classes<Policy, Character, Warrior, Object, Axe, Bear>
        YOMM2_GENSYM;

    using kick =
        method<void, std::string(virtual_ptr<Character, Policy>), Policy>;

    struct kick_definition {
        static std::string fn(virtual_ptr<Bear, Policy>) {
            return std::string("growl");
        }
    };
    static typename kick::template add_definition<kick_definition> YOMM2_GENSYM;

    using fight = method<
        void,
        std::string(
            virtual_ptr<Character, Policy>, virtual_ptr<Object, Policy>,
            virtual_ptr<Character, Policy>), Policy>;

    struct fight_definition {
        static std::string
        fn(virtual_ptr<Warrior, Policy>, virtual_ptr<Axe, Policy>,
           virtual_ptr<Bear, Policy>) {
            return "kill bear";
        }
    };
    static typename fight::template add_definition<fight_definition>
        YOMM2_GENSYM;

    detail::update_methods(Policy::catalog, Policy::context);

    Bear bear;
    BOOST_TEST(kick::fn(virtual_ptr<Character, Policy>(bear)) == "growl");

    Warrior warrior;
    Axe axe;
    BOOST_TEST(fight::fn(warrior, axe, bear) == "kill bear");
}

} // namespace test_virtual_ptr_dispatch
