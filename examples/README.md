# Examples

* [accept_no_visitors](accept_no_visitors.cpp)  
Inspired by [a talk by Yuriy
Solodkyy](https://www.youtube.com/watch?v=QhJguzpZOrk&t=1467s), this example
shows how open methods are better than visitor at traversing ASTs.


* [adventure](adventure.cpp)  
  Yes, YOMM2 methods can have more than two virtual parameters. This example is
  inspired by a scene from[Colossal Cave
  Adventure](https://en.wikipedia.org/wiki/Colossal_Cave_Adventure), where a
  Player uses an Object to fight a Foe (thus, a triple dispatch):
  - KILL DRAGON
  - WITH WHAT? YOUR BARE HANDS?
  - YES
  - CONGRATULATIONS! YOU HAVE JUST VANQUISHED A DRAGON WITH YOUR BARE HANDS!
    (UNBELIEVABLE, ISN'T IT?)

* [cmakeyomm2](cmakeyomm2)  
Demonstrates how to use YOMM2 in a cmake project.

* [asteroids](asteroids.cpp)  
Many explanations of multi-methods use the
[Asteroids](https://en.wikipedia.org/wiki/Asteroids_(video_game)) video game as
an example. Here is the YOMM2 version.


* [dl_main](dl_main.cpp), [dl_shared](dl_shared.cpp)  
Demonstrates how to dynamically load new classes and method definitions.


* [matrix](matrix.cpp)  
  Another example of double dispatch.


* [next](next.cpp)  
Demonstrates how a method definition can call the next most specialised
definition. This is similar to a virtual function override calling the base
version of the function, or calling `super` in languages like Smalltalk or
Python.

* [synopsis](synopsis.cpp)  
  A heavily annotated example, for people who prefer to learn by reading code,
  not documentation. At the bottom of the file, the assembler code generated for
  calling 1- and a 2-virtual argument methods is listed and commented.
