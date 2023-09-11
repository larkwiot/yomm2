<!-- target:reference -->
<sub>/ [home](/README.md) </sub>

# Reference

## Introduction

YOMM2 is a library that implements open multi-methods for C++17. The semantics
of methods are strongly influenced by the [Open Multi-Methods for
C++](https://www.stroustrup.com/multimethods.pdf) paper by Peter Pirkelbauer,
Yuriy Solodkyy, and Bjarne Stroustrup.

This implementation diverges from the paper on the following points:
* A "base-method" is called a "method declaration" in YOMM2, and an "overrider"
  is called a "method definition". This is because
* YOMM2 has a mechanism (`next`) to call the next most specialised method.
* The paper allows only references for virtual parameters. YOMM2 also allows
  pointers and smart pointers.
* YOMM2 does not support repeated inheritance. Multiple and virtual inheritance
  are supported.
* Method definitions are hidden. It is possible to access definitions while
  bypassing method dispatch, but this requires the definitions to be placed in a
  method container; see [the documentation](method_container.md) of
  `method_container`.

## Concepts

A *method* is a function that has one or more virtual parameters, and a
collection of method definitions.

A *method definition* is a function attached to a single method. The type of the
parameters corresponding to the virtual parameters in the method must be
compatible with the method's parameter type. The other parameters must have the
same type as in the method. The return type of the definition must be compatible
with the return type of the method.

A *virtual parameter* is a parameter that is taken into account when the method
is called.

A *virtual argument* is an argument in a method call that corresponds to a
virtual parameter in the method declaration. The method uses the *dynamic* type
of the virtual arguments to select which definition to execute. The rules for
selecting the definition are the same as for overload resolution: use the most
specialised definition, from the set of applicable definitions. However, note
that the selection happens at runtime.

## API Overview

The library is normally used via the _keyword interface_, provided by the
`<yorel/yomm2/keywords.hpp>` header. It attempts to present open methods as a
language feature. It consists of a collection of macros, which are, of course,
global, but so are keywords. The [virtual_](virtual_.md) template, used to specify
virtual parameters, is also aliases in the global namespace.

The library can also be used through the _core interface_, which is almost
entirely free of macros. The primary use of this interface is to support
templatized classes, methods and definitions - something that macros are
incapable of. See [the templates tutorial](tutorials/templates_tutorial.md) for
more details and examples.

## Exceptions

YOMM2 is exception agnostic. The library does not throw nor catches exceptions,
but it is exception safe. Errors are reported via an indirect call to a handler
function, which can be set with [set_error_handler](set_error_handler.md). A handler may throw
exceptions.

## Headers

### `<yorel/yomm2/keywords.hpp>`

This header provides the _keyword interface_. Since version 1.3.0, this is the
recommended main header for normal usage of the library. It is used in most
examples and tutorials. The header makes it look like the library's features are
part of the language:

* It includes `<yorel/yomm2/core.hpp>`.
* It includes `<yorel/yomm2/cute.hpp>`, thus making the lowercase macros
  (`declare_method`, etc) available.
* It contains a `using ::yorel::yomm2::virtual_` directive.

### `<yorel/yomm2/core.hpp>`

This header provides the _core interface_, in the `yorel::yomm2` namespace.
Since version 1.3.0, the key mechanisms are documented; thus, it possible to use
the library without resorting on the macros. See the [API
tutorial](../tutorials/api.md) for an introduction to the main features of
`core`.

The main constructs are:

* [method](method.md), a class template that contains:
  * a static function object [fn](method.md#fn), to call the method
  * nested class templates [add_function](method.md#add_function) and
    [add_definition](method.md#add_definition), to add definitions to a method
  * [next_type](method.md#next_type) and [use_next](method.md#use_next), to call
    the next most specialised method
* [use_classes](use_classes.md), a class template, provides the class and inheritance information.
* [update_methods](update_methods.md), a function that calculates the method dispatch tables, using
  the method, definition, and class information.

The header itself does not define any macros, except for its include guard
(`YOREL_YOMM2_CORE_INCLUDED`).

The header consumes two macros:
* `NDEBUG`: if defined, no checks are performed during method calls. This
  delivers a performance close to normal virtual function calls.
* `YOMM2_ENABLE_TRACE`: enables tracing. This feature is not documented at the
  moment.
* `YOMM2_SHARED`: if defined, the library runtime is in a shared library or DLL.

The header defines the following macros:
* an include guard (`YOREL_YOMM2_CORE_INCLUDED`).
* *iff* `YOMM2_SHARED` is defined, a `yOMM2_API` macro, for internal use.

### `<yorel/yomm2/symbols.hpp>`

This header defines two macros: `YOMM2_GENSYM`, which generates a new obfuscated
symbol each time that is is expanded; and `YOMM2_SYMBOL(seed)`, which generates
an obfuscated symbol (the same symbol for the same value of `seed`).

These macros are useful when using the core interface, which requires
instantiating static objects to register classes, methods, and definitions; and
for defining the "key" type for the [method](method.md) template.

### `<yorel/yomm2/templates.hpp>`

This header defines *experimental* meta-programming constructs intended to
facilitate the creation of templatized method and methods definitions. See the
[template tutorial](../tutorials/templates_tutorial.md) for examples.

### `<yorel/yomm2/macros.hpp>`

This header defines the upper-case versions of the macros (`YOMM2_DECLARE` etc).

### `<yorel/yomm2/cute.hpp>`

This header defines the lower-case versions of the macros (`declare_method`
etc).

### `<yorel/yomm2.hpp>`

This was the recommended header before version 1.3.0. Includes
`<yorel/yomm2/core.hpp>` and `<yorel/yomm2/macros.hpp>`.

## Libraries

40488
99424
~58K

## Index

| name                             | type              | purpose                                                                  |
| -------------------------------- | ----------------- | ------------------------------------------------------------------------ |
| [class_declaration](class_declaration.md)              | class template    | declares a class and its bases                                           |
| [declare_method](declare_method.md)                 | macro             | declares a method                                                        |
| [declare_static_method](declare_static_method.md)          | macro             | declares a static method inside a class                                  |
| [define_method](define_method.md)                  | macro             | adds a definition to a method                                            |
| [define_method_inline](define_method_inline.md)           | macro             | adds an definition to a method in a container, and make it inline        |
| [derived](intrusive_modes.md)                        | class template    | helper for intrusive modes                                               |
| [error_handler_type](None)             | type              | handler function                                                         |
| [error_type](None)                     | variant           | object passed to error handler                                           |
| [friend_method](friend_method.md)                  | macro             | makes a method in a container, or the entire container, a friend         |
| [hash_search_error](None)              | class             | failure to find a hash function for registered classes                   |
| [make_virtual_shared](virtual_ptr.md)            | function template | creates an object and return a `virtual_shared_ptr`                      |
| [method](method.md)                         | class template    | implements a method                                                      |
| [method_call_error](method_call_error.md)              | class             | information about a failed method call                                   |
| [method_call_error_handler](method_call_error.md)      | type              | type of a function called when a method call fails                       |
| [method_container](method_container.md)               | macro             | declares a method definition container                                   |
| [method_definition](method_definition.md)              | macro             | retrieves a definition from a container                                  |
| [register_class](register_class.md)                 | macro             | registers a class and its bases (deprecated)                             |
| [register_classes](use_classes.md)               | macro             | registers classes and their inheritance relationships                    |
| [resolution_error](None)               | class             | method call does not resolve to exactly one definition                   |
| [root](intrusive_modes.md)                           | class template    | helper for intrusive modes                                               |
| [set_error_handler](set_error_handler.md)              | function          | sets the function called for all errors                                  |
| [set_method_call_error_handler](method_call_error.md)  | function          | sets function to call when a method call fails                           |
| [unknown_class_error](None)            | class             | class used in method declaration, definition, or call was not registered |
| [update](update.md)                         | function          | sets up dispatch tables                                                  |
| [update_methods](update_methods.md)                 | function          | sets up dispatch tables (deprecated, requires linking with library)      |
| [use_classes](use_classes.md)                    | class template    | registers classes and their inheritance relationships                    |
| [virtual_](virtual_.md)                       | class template    | marks a method parameter as virtual                                      |
| [virtual_ptr](virtual_ptr.md)                    | class template    | fat pointer for optimal method dispatch                                  |
| [virtual_shared_ptr](virtual_ptr.md)             | class template    | `virtual_ptr` using a `std::shared_ptr`                                  |
| [YOMM2_CLASS](register_class.md)                    | macro             | same as `register_class` (deprecated)                                    |
| [YOMM2_CLASSES](use_classes.md)                  | macro             | same as `register_classes`                                               |
| [YOMM2_DECLARE](declare_method.md)                  | macro             | same as `declare_method`                                                 |
| [YOMM2_DECLARE_METHOD_CONTAINER](method_container.md) | macro             | same as `method_container`                                               |
| [YOMM2_DEFINE](define_method.md)                   | macro             | same as `define_method`                                                  |
| [YOMM2_DEFINE_INLINE](define_method_inline.md)            | macro             | same as `define_method_inline`                                           |
| [YOMM2_DEFINITION](method_definition.md)               | macro             | same as `method_definition`                                              |
| [YOMM2_FRIEND](friend_method.md)                   | macro             | same as `friend_method`                                                  |
| [YOMM2_GENSYM](YOMM2_GENSYM.md)                   | macro             | generates a unique symbol                                                |
| [YOMM2_STATIC_DECLARE](declare_static_method.md)           | macro             | declares a static method inside a class                                  |
| [YOMM2_SYMBOL](YOMM2_SYMBOL.md)                   | macro             | generates an obfuscated symbol                                           |



### *Experimental* template helpers

| name              | type           | purpose                                                          |
| ----------------- | -------------- | ---------------------------------------------------------------- |
| [apply_product](apply_product.md)   | meta-function  | apply templates to the n-fold Cartesian product of `types` lists |
| [not_defined](not_defined.md)     | meta-function  | tell [use_definitions](use_definitions.md) to discard a definition                   |
| [product](product.md)         | meta-function  | form n-fold Cartesian product of `types` lists                   |
| [template_](template_.md)       | class template | wrap a template in a type                                        |
| [templates](templates.md)       | class template | wrap templates in a `types` list                                 |
| [types](types.md)           | class template | sequence of types                                                |
| [use_definitions](use_definitions.md) | class template | add batch of definitions from a generic container to methods     |
