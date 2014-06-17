# Some design notes

##Aggregation functions

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

    - When the query return no rows only final is called once and it would
        generally be unnecessary to allocate STATE, since no data needs to
        persist to future funciton calls (there are no future function calls).
        
        If FINAL is a method of a STATE object, the object needs to be
        instantiated anyway.

    S* create();
    step(S*, value);
    final(S*);
    destroy(S*);

class weighted_avg {
    void step(double value) {
        avg = avg * (1.0-w) + value * w;
    }
    double final() {
        return avg;
    }
};



