# Memlay
Memory Layout Parser

`memlay` prints the memory application's layout (i.e. text, data, heap and stack segments) in a given breakpoint of execution. It is very similar to `pmap`, since it reads the `/proc/<pid>/maps`file.

# Implementation

For instance, the breakpoint may be any symbol defined for local functions in user's application. This includes statically linked libraries, since they will part of the final executable. On the other hand, a breakpoint cannot be defined for those functions from external (dynamic) libraries.

The program works by doing a binary analysis of the executable (must be a ELF file) to set the breakpoint and find the symbol in the symbol table (.symtab section). Thus, the user's program may be compiled without the '-g' flag.

Memlay was not extensively tested.

# Current features

Ready for one breakpoint analysis.

Implementation of more breakpoints on progress.

# Usage
memlay \<breakpoint\> \<executable\> \<args\>
