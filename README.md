# Memlay
Memory Layout Parser

`memlay` prints the memory application's layout (i.e. text, data, heap and stack segments) in a given breakpoint of execution. It is very similar to `pmap`, since it reads the `/proc/<pid>/maps`file.

# Implementation

For instance, the breakpoint may be any symbol defined for local functions in user's application. This includes statically linked libraries, since they will part of the final executable. On the other hand, a breakpoint cannot be defined for those functions from external (dynamic) libraries.

The program uses de `libdwarf` to retrive the function's address from DWARF format and then set the breakpoint. Thus, the user's program must be compiled with the '-g' flag.

Memlay was not extensively tested.

# Current features

Implementation of the DWARF analysis feature on progress.

# Usage
memlay \<breakpoint\> \<executable\> \<args\>
