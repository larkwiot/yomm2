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

    static use_classes<Policy, Character, Bear> YOMM2_GENSYM;
    detail::update_methods(Policy::catalog, Policy::context);

    using vptr_character = virtual_ptr<Character, Policy>;
    static_assert(detail::is_virtual_ptr<vptr_character>);
    using vptr_cat = virtual_ptr<Bear, Policy>;

    Character character;
    auto virtual_character = vptr_character::final(character);
    BOOST_TEST(&virtual_character.object() == &character);
    BOOST_TEST(
        (virtual_character.method_table() == method_table<Character, Policy>));

    Bear bear;
    BOOST_TEST((&vptr_cat::final(bear).object()) == &bear);
    BOOST_TEST(
        (vptr_cat::final(bear).method_table() == method_table<Bear, Policy>));

    BOOST_TEST(
        (vptr_character(bear).method_table() == method_table<Bear, Policy>));

    vptr_cat virtual_cat_ptr(bear);
    vptr_character virtual_character_ptr = virtual_cat_ptr;

    struct upcast {
        static void fn(vptr_character) {
        }
    };

    upcast::fn(virtual_cat_ptr);

    static method<Character, std::string(virtual_<Character&>), Policy>
        YOMM2_GENSYM;
    detail::update_methods(Policy::catalog, Policy::context);
    BOOST_TEST(
        (virtual_cat_ptr.method_table() == method_table<Bear, Policy>) ==
        vptr_character::is_indirect);
}
} // namespace test_virtual_ptr

#ifndef NDEBUG

namespace bad_virtual_ptr {

struct key {};
using test_policy = test_policy_<key, false>;

register_classes(test_policy, Character, Bear);

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
        Bear paddington;
        Character& animal = paddington;
        virtual_ptr<Character, test_policy>::final(animal);
        set_error_handler(prev_handler);
    } catch (const method_table_error& error) {
        BOOST_TEST(error.ti->name() == typeid(Bear).name());
        set_error_handler(prev_handler);
        return;
    } catch (...) {
        set_error_handler(prev_handler);
        BOOST_FAIL("wrong exception");
    }

    BOOST_FAIL("did not throw");

    set_error_handler(prev_handler);
}
} // namespace bad_virtual_ptr

#endif

namespace test_virtual_ptr_dispatch {

struct key {};

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
            virtual_ptr<Character, Policy>),
        Policy>;

    struct fight_definition {
        static std::string
        fn(virtual_ptr<Warrior, Policy>, virtual_ptr<Axe, Policy>,
           virtual_ptr<Bear, Policy>) {
            return "kill bear";
        }
    };
    static typename fight::template add_definition<fight_definition>
        YOMM2_GENSYM;

    detail::log_on(&std::cerr);
    detail::update_methods(Policy::catalog, Policy::context);

    Bear bear;
    BOOST_TEST(kick::fn(virtual_ptr<Character, Policy>(bear)) == "growl");

    Warrior warrior;
    Axe axe;
    BOOST_TEST(
        fight::fn(
            virtual_ptr<Character, Policy>(warrior),
            virtual_ptr<Object, Policy>(axe),
            virtual_ptr<Character, Policy>(bear)) == "kill bear");
}

} // namespace test_virtual_ptr_dispatch
