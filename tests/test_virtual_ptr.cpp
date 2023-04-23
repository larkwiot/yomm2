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

register_classes(test_policy, Animal, Cat);

BOOST_AUTO_TEST_CASE_TEMPLATE(
    test_virtual_ptr, Indirection, indirection_types) {
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

struct Dog : Animal {};

register_classes(Animal, Dog);

declare_method(std::string, kick, (virtual_<Animal&>));

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
        virtual_ptr<Animal>::final(snoopy);
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

struct YOMM2_SYMBOL(kick);
using kick = method<YOMM2_SYMBOL(kick), std::string(virtual_ptr<Character>)>;

static auto definition(virtual_ptr<Bear>) {
    return std::string("growl");
}

typename kick::template add_function<definition> YOMM2_GENSYM;

BOOST_AUTO_TEST_CASE_TEMPLATE(
    test_virtual_ptr_dispatch_uni_method, IndirectionType, indirection_types) {
    // detail::trace_flags = 3;
    // detail::logs = &std::cerr;
    update_methods();

    Bear bear;
    BOOST_TEST(kick::fn(virtual_ptr<Character>(bear)) == "growl");
}

#if 0

template<typename Characters, typename Devices, typename Creatures>
using fight = method<
    void,
    std::string(
        virtual_<typename Characters::Character&>,
        virtual_<typename Devices::Object&>,
        virtual_<typename Creatures::Creature&>)>;

template<typename Characters, typename Devices, typename Creatures>
std::string kill_bear(
    typename Characters::Warrior&, typename Devices::Axe&,
    typename Creatures::Bear&) {
    return "kill bear";
}
using class_set_product = boost::mp11::mp_product<
    std::tuple, class_set_list, class_set_list, class_set_list>;

BOOST_AUTO_TEST_CASE_TEMPLATE(
    test_intrusive_calls_multi, Tuple, class_set_product) {
    // detail::trace_flags = 3;
    // detail::logs = &std::cerr;
    using Characters = boost::mp11::mp_first<Tuple>;
    using Devices = boost::mp11::mp_second<Tuple>;
    using Creatures = boost::mp11::mp_third<Tuple>;

    using fight_type = fight<Characters, Devices, Creatures>;
    static typename fight_type::template add_function<
        kill_bear<Characters, Devices, Creatures>>
        YOMM2_GENSYM;
    update_methods();

    typename Characters::Warrior warrior;
    typename Devices::Axe axe;
    typename Creatures::Bear bear;
    BOOST_TEST(fight_type::fn(warrior, axe, bear) == "kill bear");
}

#endif
} // namespace test_virtual_ptr_dispatch