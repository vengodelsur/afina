## Memo for the newbie author

#### Why use reference_wrapper?
~~Compiler complains of references stored in container~~ References are not objects and have no adress => pointer to the element type can't be formed => element type is not [erasable](http://en.cppreference.com/w/cpp/concept/Erasable).

#### Well, what is an object, then?

An object is a region of storage that has

* size (can be determined with sizeof);
* alignment requirement (can be determined with alignof);
* storage duration (automatic, static, dynamic, thread-local);
* lifetime (bounded by storage duration or temporary);
* type;
* value (which may be indeterminate, e.g. for default-initialized non-class types);
* optionally, a name.

#### Why not use raw pointers?

"Using a reference_wrapper essentially disavows ownership of the object". [Stackoverflow](https://stackoverflow.com/questions/26766939/difference-between-stdreference-wrapper-and-simple-pointer)
