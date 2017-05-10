# Some design notes

Random design notes and considerations/tradeoffs considered when designing
the library.

## Functions overloaded for data types supported by sqlite

Many functions, like `statement::bind` come in basically six variations,
supporting a `int`, `int64_t`, `double`, `const char*`, `std::string` and
`blob` as parameter types. This could be done by simply defining six
overloads of the function in question, one for each type. No templates are
necessary:

    void bind(int idx, int value);
	 void bind(int idx, int64_t value);
	 void bind(int idx, double value);
	 ...

A call to `bind()` with one of those types will select the correct overload.

There is one problem, though: If the called datatype doesn't match one of the
overloaded types exactly it's easy to run into ambiguous overloads:

    size_t s = ...;
    bind(0, s); // error: ambigous conversion

One solution is to cast the ambiguous type, one way or another:

	 bind(0, int(s));
	 bind(0, static_cast<int>(s));

Problem: A simple C-style cast like in the first line will do all kinds of
unintended conversions. For example `bind(0, int(s));` will run without warning
if `s` is a pointer. Maybe `s` meant "string", not "size". A `static_cast<>()`
like in the second line should be safe and do what we want, but it's rather
verbose.

Using a template for bind instead has the advantage that we can specify the
desired type directly if required to avoid ambiguity:

	 template<typename T>
    void bind(int idx, T value);
   
	 bind<int>(0, size);

The disadvantage is that nothing will implicitly convert to the correct overloaded
type, even if there would only be one possible conversion. This is even the case
if we use `enable_if` or similar machinery to only allow the desired types. (At
least I think so. A template instantiation is attempted for the parameter type, and
if it fails, there won't be additional attempts to instantiate the template for all
possible conversions. But maybe we should test it to be sure (TODO).)

If we provide *both* the "simple" overloads and the template we might hope to get
the best of both wolds, but really the non-template functions will only be preferred
if they are exact matches (see also http://www.gotw.ca/gotw/049.htm question 3.E).
(TODO: Test anyway)

One alternative is to provide both under different names:

    void bind(int idx, int value);
	 void bind(int idx, int64_t value);
	 void bind(int idx, double value);
	 ...
    
	 template<typename T> // TODO: add enable_if<> to only allow selected types
	 void bind_t(int idx, T &&value);

A normal `bind(s)` call will do normal overload resolution. If there is ambiguity
you have the choice of either using an explicit cast `bind(static_value<int>(s))`,
or using the slightly more convenient template function `bind_t<int>(v)`.

If we go this way we need a good naming convention for this kind of template.
`bind_t` is bad because a `_t` suffix usually means "type", not "template".
Additionally at least some names (maybe all?) with `_t` suffix are
reserved names (not sure if this also applies in namespaces/classes).

## `statement::bind()` and other function that might have to copy strings/blobs

When a string/blob needs to be passed to sqlite we generally have three options:

1. Let sqlite make a copy of the data's memory with `SQLITE_TRANSIENT`
2. Tell sqlite the memory will live long enough with `SQLITE_STATIC`
3. Let sqlite take ownership of the memory and have a destructor called
    when it's no longer needed

For both 1. and 2. we would get a `const&` or `const*` and we can't tell them
apart, so we need an extra boolean flag that tells us what to do, if we want to
support both cases. The safe option is always to make a copy (1.), which should be
default. Avoiding the copy (2.) is a performance optimization that should be
available, but can't be default since it is not generally safe to do.

For 3. we can take a `&&` parameter. If the data is moved into out function
we own it and can pass it on to sqlite with a appropriate destructor.
But this also comes with problems:

- If we get ownership of a C string as `char *&&` we need to know how to deallocate
    it. The C++
    way would be to allocate it with `c = new char[SIZE];` which we have to deallocate
    with `delete[] c;`. But is this commonly done? The *real* C++ way to allocate a
    string would be to use `std::string` instead.

    If the passed `char*` was allocated with `malloc()` we can't `delete[]` it, we
    would have to use `free()`.  
    We could:

    - Document we require a `new[]` allocated string. This is not optimal since the whole
        point of this interface would be to avoid the cost of copying the string. If C-like
        strings don't commonly are allocated this way the whole interface is useless and we
        have to pay for copies we wouldn't need.
    - Add an extra destructor argument that could or has to be passed. If it is optional,
        it will be forgotten and lead to nasty bugs, if it is required it will be a hassle,
        especially since you can't pass `delete[]` directly as a destructor function.
        Is this worth the more complicated interface? Is moving in a `char*` esoteric enough
        that whoever attempts to do it is probably willing to learn about custom destructor
        functions he needs to pass as a parameter?
    - Don't support `char&&` at all, but have some other way to pass string ownership. (It
        seems like this option doesn't solve anything but only handwaves it to "some other
        way", where it will have to be solved anyway. So we as well can come to a conclusion
        right here.)
    - Don't support passing `char*` ownership. Not desirable since it adds unnecessary costs.

## Aggregate functions

When an aggregate function for a query is executed, we want:
- A function that is called for every row, aggregating data [STEP]
- A function called in the end, returning the aggregated result [FINAL]
- Some state for this aggregation that is being passed between these calls. [DATA]
    - A new state has to be created for each aggregation
    - The state has to be destroyed again after the end of the aggregation
- Additionally there *might* be some global state on which STEP and FINAL depend. [GLOBAL]

Why is that?
- Separate STEP and FINAL are not technically *required*, since each
    call to STEP could just always return the final value for the data we have seen.
    But can do lots of unnecessary computation that can be avoided with a separate FINAL.

    For example when aggregating *sqrt(a^2 + b^2 + c^2 + ....)* you update the
    sum of squares in each STEP and calculate the *sqrt()* in FINAL.

    Without a separata FINAL you'd need to return *sqrt(sum)* in each STEP, doing many
    unnecessary *sqrt()* calculations.

    This is also how the C API interface works.

- An alternative would be to only have a single function, but pass additional
    parameters that determine if it's the final call/... This isn't optimal
    since `step()`/`final()` have different parameters and return values.

- An "aggregation function" is more complicated than a simple function (it
    needs to contain STEP and FINAL and something to manage STATE).

    Straight forward would be to have a class that encapsulates STATE and
    has methods for STEP and FINAL. When a query starts an aggregation, a
    new instance of that class is created, that instance is used for STEP
    and FINAL, and after that the instance is discarded again.

    Disadvantages:

    - When the query returns no rows only FINAL is called once and it would
        generally be unnecessary to allocate STATE, since no data needs to
        persist to future funciton calls (there are no future function calls).
        
        If FINAL is a method of a STATE object, the object needs to be
        instantiated anyway.

- Have a base class with virtual functions, to register an aggregate derive a class
    and pass that:

        struct aggregation_data {
           virtual void step(...);
           virtual void final();
        }

    - `step` might take different kinds/number of parameters and return different
         types for different aggregates. This means there is no struct that works
         as a base class for all aggregates, specialization can't be done by virtual
         functions.

         So the approach doesn't work.

    - Additionally, if the aggregation data contains STATE, a new object needs
        to be created for each instance of the aggreagte. To do so, just knowing about
        the base class won't be enough. The connection class will have to know
        about the specific derived class it should instantiate, or there needs to be a factory function
        that provides the objects.

## Registering different kinds of callbacks

In the sqlite3 C API there functions that register callbacks/handlers for
different events. They have wildly varying names. The corresponding methods on
the connection class are all named as `connection::set_..._handler()` instead,
following the inconsistency of the C API didn't seem useful. The main part of
the name has been kept the same, so searching for the significant part of the C
API function name should bring up the corresponding sqxx function.

For example `sqlite3_rollback_hook` corresponds to `connection::set_rollback_handler`
and `sqlite3_collation_needed` is `connection::set_collation_handler`.

