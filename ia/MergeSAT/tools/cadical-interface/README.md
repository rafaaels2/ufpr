This directory demonstates how to use MergeSat as a CaDiCal replacement.
The source code of CaDiCaL can be found at
https://github.com/arminbiere/cadical

## Preparation

To be able to use MergeSat as a solving backend, the solver needs to be
compiled. For development and debugging, use the 'ld' compilation target,
while for an actual release, use the 'lr' compilation target.

```
make lr
```

## Using C Interface of CaDiCaL from MergeSat

### Including Declarations

To use MergeSat as a CaDiCaL replacement, you need to include the following
file:

```
#include "simp/ccadical-mergesat-bridge.h"
```

Make sure to provide MergeSat's `minisat` path to the compiler invocation
line.

### Linking Implementation

Finally, when compiling your application with the solver, make sure you
link against the previously compiled MergeSat library. This is done by
specifying the path to consume the library from (depending on the above
build target, this might be `build/debug/lib`). Furthermore, you need to
actually link against the MergeSat library. The following parameters need
to be added to your link step:

```
-L $MERGESAT_SRC/build/debug/lib -lmergesat
```