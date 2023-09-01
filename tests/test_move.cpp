#include <yorel/yomm2/keywords.hpp>

#define BOOST_TEST_MODULE yomm2
#include <boost/test/included/unit_test.hpp>
#include <boost/utility/identity_type.hpp>

enum Status { ORIGINAL, COPY, MOVED, DEAD };

struct Base {
    Status status;
    Base() : status(ORIGINAL) {
    }
    Base(const Base&) : status(COPY) {
    }
    Base(Base&& other) : status(MOVED) {
        other.status = DEAD;
    }
    virtual ~Base() {
    }
};

struct Derived : Base {};

BOOST_AUTO_TEST_CASE_TEMPLATE(
    test_moveable, Class, BOOST_IDENTITY_TYPE((std::tuple<Base, Derived>))) {
    Class a;
    BOOST_REQUIRE(a.status == ORIGINAL);

    Class b = a;
    BOOST_REQUIRE(a.status == ORIGINAL);
    BOOST_REQUIRE(b.status == COPY);

    Class c = std::move(a);
    BOOST_REQUIRE(a.status == DEAD);
    BOOST_REQUIRE(c.status == MOVED);
}

register_classes(Base, Derived);

declare_method(void, move_non_virtual_arg, (virtual_<Base&>, Base&&));

define_method(void, move_non_virtual_arg, (Base& varg, Base&& moving)) {
    BOOST_TEST(varg.status == ORIGINAL);
    BOOST_TEST(moving.status == ORIGINAL);
    static_assert(std::is_same_v<decltype(moving), Base&&>);
    Base moved = std::move(moving);
}

BOOST_AUTO_TEST_CASE(move_non_virtual_arg) {
    yorel::yomm2::update();

    Base varg, moving;
    move_non_virtual_arg(varg, std::move(moving));
    BOOST_TEST(moving.status == DEAD);
}
