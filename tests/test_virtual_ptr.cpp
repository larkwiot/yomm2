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

namespace direct_virtual_ptr {

struct Animal {
    virtual ~Animal() {
    }
};

struct Cat : Animal {};

register_classes(Animal, Cat);

BOOST_AUTO_TEST_CASE(test_direct_virtual_ptr) {
    update_methods();

    static_assert(detail::is_virtual_ptr<virtual_ptr<Animal>>);

    Animal animal;
    auto virtual_animal = virtual_ptr<Animal>::final(animal);
    BOOST_TEST(&virtual_animal.object() == &animal);
    BOOST_TEST(virtual_animal._mptr() == method_table<Animal>);

    Cat cat;
    BOOST_TEST(&virtual_ptr<Cat>::final(cat).object() == &cat);
    BOOST_TEST(virtual_ptr<Cat>::final(cat)._mptr() == method_table<Cat>);

    BOOST_TEST(virtual_ptr<Animal>(cat)._mptr() == method_table<Cat>);
}
} // namespace direct_virtual_ptr

namespace indirect_virtual_ptr {

struct Animal {
    virtual ~Animal() {
    }
};
struct Cat : Animal {};

register_classes(Animal, Cat);

BOOST_AUTO_TEST_CASE(test_indirect_virtual_ptr) {
    update_methods();

    static_assert(detail::is_virtual_ptr<virtual_ptr<Animal, indirect>>);

    Animal animal;
    auto virtual_animal = virtual_ptr<Animal, indirect>::final(animal);
    BOOST_TEST(&virtual_animal.object() == &animal);
    BOOST_TEST(virtual_animal._mptr() == &method_table<Animal>);

    Cat cat;
    BOOST_TEST((&virtual_ptr<Cat, indirect>::final(cat).object() == &cat));
    BOOST_TEST(
        (virtual_ptr<Cat, indirect>::final(cat)._mptr() == &method_table<Cat>));

    BOOST_TEST(
        (virtual_ptr<Animal, indirect>(cat)._mptr() == &method_table<Cat>));
}
} // namespace indirect_virtual_ptr

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

#if 0

struct indirect_intrusive_mode {
    template<typename Class>
    using root = root<Class, indirect>;
    template<typename Class>
    using derived = derived<Class>;
};

struct direct_intrusive_mode {
    template<typename Class>
    using root = root<Class, direct>;
    template<typename Class>
    using derived = derived<Class>;
};

template<typename Class>
struct no__mptr {};

struct orthogonal_mode {
    template<typename Class>
    using root = no__mptr<Class>;
    template<typename Class>
    using derived = no__mptr<Class>;
};

template<typename Mode>
struct class_set {
    template<typename Class>
    using root = typename Mode::template root<Class>;
    template<typename Class>
    using derived = typename Mode::template derived<Class>;

    struct Character : root<Character> {
        virtual ~Character() {
        }
    };

    struct Warrior : Character, derived<Warrior> {};

    struct Device : root<Device> {
        virtual ~Device() {
        }
    };

    struct Axe : Device, derived<Axe> {};

    struct Creature : root<Creature> {
        virtual ~Creature() {
        }
    };

    struct Bear : Creature, derived<Bear> {};

    use_classes<Character, Warrior, Device, Axe, Creature, Bear> YOMM2_GENSYM;

    struct YOMM2_SYMBOL(kick);
    using kick = method<YOMM2_SYMBOL(kick), std::string(virtual_<Creature&>)>;

    static auto definition(Bear&) {
        return std::string("growl");
    }

    typename kick::template add_function<definition> YOMM2_GENSYM;
};

using mode_list =
    std::tuple<indirect_intrusive_mode, direct_intrusive_mode, orthogonal_mode>;

using class_set_list = boost::mp11::mp_transform<class_set, mode_list>;
boost::mp11::mp_apply<std::tuple, class_set_list> YOMM2_GENSYM;

BOOST_AUTO_TEST_CASE_TEMPLATE(
    test_intrusive_calls_uni, ClassSet, class_set_list) {
    // detail::trace_flags = 3;
    // detail::logs = &std::cerr;
    update_methods();

    typename ClassSet::Bear bear;
    BOOST_TEST(ClassSet::kick::fn(bear) == "growl");
}

template<typename Characters, typename Devices, typename Creatures>
using fight = method<
    void,
    std::string(
        virtual_<typename Characters::Character&>,
        virtual_<typename Devices::Device&>,
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
