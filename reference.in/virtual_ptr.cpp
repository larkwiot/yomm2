// clang-format off

#include <yorel/yomm2/keywords.hpp>
#include <yorel/yomm2/intrusive.hpp>

#define BOOST_TEST_MODULE yomm2
#include <boost/test/included/unit_test.hpp>

#ifdef YOMM2_MD
<sub>/ ->home / ->reference </sub>

entry: yorel::yomm2::virtual_ptr
headers: yorel/yomm2/core.hpp

---
```c++

template<class Class, class Policy = yorel::yomm2::default_policy>
class virtual_ptr;
```
---
`virtual_ptr` is a fat pointer that consists of a pointer to an object, and a
pointer to its associated method table.

## member functions

|                               |                              |
| ----------------------------- | ---------------------------- |
| ([constructor](#constructor)) | constructs a new virtual_ptr |
| ([destructor](#destructor))   | destructs the virtual_ptr    |


### observers

|                               |                                 |
| ----------------------------- | ------------------------------- |
| [operator*](#deref-operator)  | dereferences the stored pointer |
| [operator->](#arrow-operator) | dereferences the stored pointer |


## static member functions

|                 |                           |
| --------------- | ------------------------- |
| [final](#final) | returns a new virtual_ptr |

### virtual_ptr&lt;Class, Policy&gt;::virtual_ptr

|                                                                                        |     |
| -------------------------------------------------------------------------------------- | --- |
| `template<class OtherClass> virtual_ptr(OtherClass& obj)`                              | (1) |
| `template<class OtherClass> virtual_ptr(const virtual_ptr<OtherClass, Policy>& other)` | (2) |

Constructs a new `virtual_ptr`, from a reference to an object or another
`virtual_ptr`.

---

For the purposes of the description below, a reference type `OtherClass&` is
said to be compatible with a reference `Class&` if `OtherClass&` is convertible
to `Class&`.

---

(1) Constructs a `virtual_ptr` that contains a reference to `obj`, which must be
compatible with `Class`, and a pointer to the method table corresponding to
`obj`'s *dynamic* type. If the dynamic type of `obj` is the same as `OtherClass`
(i.e. `typeid(obj) == typeid(OtherClass)`), the hash table is not looked up.

(2) Constructs a `virtual_ptr` that contains a reference to `obj`, and a pointer
to the method table corresponding to `obj`'s *dynamic* type. If the dynamic type
of `obj` is the same as `OtherClass` (i.e. `typeid(obj) == typeid(OtherClass)`),
the hash table is not looked up.

`Policy` must be the same for both `virtual_ptr`s. This is because constructing
a `virtual_ptr` with a different `Policy` incurs the cost of a hash table
lookup. When needed, it can be done explicitly:

```
virtual_ptr<Animal, my_policy> my_ptr(animal);
// ...
virtual_ptr<Animal, your_policy> your_ptr(*my_ptr);
```

### static virtual_ptr&lt;Class, Policy&gt;>final&lt;OtherClass&gt;

Returns a `virtual_ptr` that contains a reference to `obj`, and a pointer
to the method table for `OtherClass`.

If `Policy::enable_runtime_checks` is `true` (the default in debug builds), and
the dynamic type of `obj` is differs from `OtherClass`, the current error
handler is called with a `method_table_error` variant.

## Notes

Calls to methods through a `virtual_ptr` are almost as efficient as virtual
function calls: three instructions and two memory reads, vs two instructions and
one memory read for virtual functions. This is because they do not incur the
cost a hash table lookup, unlike calls made using the orthogonal mode. Instead,
the lookup is done once, when the pointer is constructed, or not at all, when
using `virtual_ptr::final`.

In spite of the 'ptr' in its name, `virtual_ptr` should be viewed more like a
*reference*, because it is not possible to create a null `virtual_ptr` [^1].

## Direct vs Indirect

## Examples

#endif

namespace direct_intrusive {

#ifdef YOMM2_CODE

using yorel::yomm2::virtual_ptr;

class Animal {
  public:
    virtual ~Animal() {
    }
};

class Dog : public Animal {
};

declare_method(void, kick, (virtual_ptr<Animal>));

declare_method(
    void, meet, (virtual_ptr<Animal>, virtual_ptr<Animal>));

define_method(void, kick, (virtual_ptr<Dog> dog)) {
    std::cout << "bark";
}

define_method(
    void, meet, (virtual_ptr<Dog> a, virtual_ptr<Dog> b)) {
    std::cout << "wag tail";
}

#endif

void call_kick(virtual_ptr<Animal> animal) {
    kick(animal);
}

void call_meet(virtual_ptr<Animal> a, virtual_ptr<Animal> b) {
    meet(a, b);
}

register_classes(Animal, Dog);

BOOST_AUTO_TEST_CASE(reference_direct_intrusive) {
    yorel::yomm2::update_methods();
    Dog dog;
    call_kick(dog);
    call_meet(dog, dog);
}

} // namespace direct_intrusive

#if 0











#ifdef YOMM2_MD

A call to `kick` compiles to just three instructions:

```
	mov	rax, qword ptr [rip + method<kick, ...>::fn+96]
	mov	rax, qword ptr [rsi + 8*rax]
	jmp	rax
```

A call to `meet` compiles to:

```
	mov	rax, qword ptr [rip + method<meet, ...>::fn+96]
	mov	r8, qword ptr [rsi + 8*rax]
	mov	rax, qword ptr [rip + method<meet, ...>::fn+104]
	mov	rax, qword ptr [rcx + 8*rax]
	imul	rax, qword ptr [rip + method<meet, ...>::fn+112]
	mov	rax, qword ptr [r8 + 8*rax]
	jmp	rax


```

`root` plants two functions, both called `yomm2_mptr`, and a pointer, with an
obfuscated name, in the target class. All classes derived from a YOMM2
`root<Class>` class must derive from `derived<Class>`. Both templates define a
default constructor that sets the method table pointer.

In debug builds, hash table lookup is always performed, and the result is
compared with the method pointer stored inside the object. If they differ, an
error is raised. This helps detect missing `derived` specifications.

The second template argument - either `direct`, the default, or `indirect` -
specifies how the method pointer is stored inside the objects. In `direct` mode,
it is a straight pointer to the method table. While such objects exist,
`update_methods` cannot be called safely (for example, after dynamically loading
a library), because the pointers would be invalidated.

In indirect mode, objects contains a pointer to a pointer to the method
table. Because of the indirection, this makes method calls slightly slower, but
`update_methods` can be safely called at any time.


For example:

#endif

namespace intrusive_multiple_inheritance {

#ifdef YOMM2_CODE
namespace yomm2 = yorel::yomm2;

struct Animal : public yomm2::root<Animal> {
public:
    virtual ~Animal() {
    }
};

class Property : public yomm2::root<Property> {
public:
    virtual ~Property() {
    }
};

class Dog : public Animal, public Property,
    public yomm2::derived<Dog, Animal, Property>
{
public:
    using yomm2::derived<Dog, Animal, Property>::yomm2_mptr;
};

struct Pitbull : Dog, yomm2::derived<Pitbull> {};

register_classes(Animal, Property, Dog, Pitbull);

declare_method(void*, pet, (virtual_<Animal&>));

define_method(void*, pet, (Pitbull & dog)) {
    return &dog;
}

declare_method(void*, sell, (virtual_<Property&>));

define_method(void*, sell, (Dog & dog)) {
    return &dog;
}

#endif

BOOST_AUTO_TEST_CASE(reference_direct_intrusive_mi) {
    yomm2::update_methods();
    Pitbull dog;
    Animal& animal = dog;
    Property& property = dog;
    BOOST_TEST(pet(animal) == &dog);
    BOOST_TEST(sell(property) == &dog);
}

} // namespace direct_intrusive

#ifdef YOMM2_MD

[^1]: The only reason why `virtual_ptr` is not called `virtual_ref` is to save
    the name for the time when C++ will support a user-defined dot operator.

#endif





#endif