
<sub>/ [home](/README.md) / [reference](README.md) </sub>

## yorel::yomm2::aware
## yorel::yomm2::direct
## yorel::yomm2::indirect
<sub>defined in header<yorel/yomm2/core.hpp></ sub>
<!-- --> 
---

```c++
struct direct;

struct indirect;

template<class Class, class Indirection = direct, typename = /*unspecified*/>
struct aware {
    yomm2_mptr(...);
};
```
---
YOMM2 uses per-class _method tables_ to dispatch calls efficiently. This is
similar to the way virtual functions are implemented. In orthogonal mode (the
default), the method table for an object is obtained from a hash table. The
hash function is collision-free, and very efficient. The overhead is ~25%
compared to virtual functions.

The hash table lookup can be eliminated for YOMM2-aware class hierarchies. This
is done by publicly inheriting from
[CRTP](https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern) class
template `yomm2::aware`:


```c++
namespace yomm2 = yorel::yomm2; // for brevity

class Animal : public yomm2::base<Animal> {
  public:
    virtual ~Animal() {
    }
};

class Dog : public Animal, public yomm2::derived<Dog> {
};

declare_method(void, kick, (virtual_<Animal&>));

declare_method(void, meet, (virtual_<Animal&>, virtual_<Animal&>));
```


A call to `kick` compiles to just three instructions:

```
	mov	rax, qword ptr [rdi + 8]
	mov	ecx, dword ptr [rip + method<kick>::fn+88]
	jmp	qword ptr [rax + 8*rcx]         # TAILCALL
```

```
	*(a.mptr[m.slot1] + b.mptr[m.slot2] * m.stride2])

	mov	rax, 	qword ptr [rdi + 8]						; a.mptr
	mov	rcx, 	qword ptr [rip + method<meet>::fn+88]   ; ssp [table, slot2, stride2]
	mov	edx, 	dword ptr [rcx]                         ; table
	mov	r8d, 	dword ptr [rcx + 8]                     ; slot2
	mov	rax, 	qword ptr [rax + 8*rdx]                 ; 
	mov	rdx, 	qword ptr [rsi + 8]
	mov	edx, 	dword ptr [rdx + 8*r8]
	imul		edx, dword ptr [rcx + 16]
	mov	rax, 	qword ptr [rax + 8*rdx]
	jmp	rax                             # TAILCALL


```

The first time `aware` appears in a hierarchy, it installs several functions,
all called `yomm2_mptr`, and a pointer, with an obfuscated name. It also defines
a constructor that sets the pointer to the method table for the class.


All the classes derived from a YOMM2-aware class need to be aware as well, if
they appear in method calls. In debug builds, the hash table lookup is still
performed, and the result is compared with the method pointer stored inside the
aware object. If they differ, an error is raised. This helps detect missing
`aware` specifications.


The second template argument - either `direct`, the default, or `indirect` -
specifies how the method pointer is stored inside the objects. In `direct` mode,
it is a straight pointer to the method table. While such objects exist,
`update_methods` cannot be called (for example, after dynamically loading a
library), because the pointers would be invalidated.

In indirect mode, the objects contains a pointer to a pointer to the method
table. Because of the indirection, yhis makes method calls slightly slower, but
`update_methods` can be safely called at any time.


```c++
namespace yomm2 = yorel::yomm2;

struct Animal : public yomm2::base<Animal> {
public:
    virtual ~Animal() {
    }
};

class Property : public yomm2::base<Property> {
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
```
