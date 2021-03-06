# Memlay
Memory Layout Parser

`memlay` prints the memory application's layout (i.e. text, data, heap and stack segments) in a given breakpoint of execution. It is similar to `pmap`, since it reads the `/proc/<pid>/maps` file.

## Compilation

`g++ -o memlay memlay.c retdwarf.c -ldwarf -lelf`

## Implementation

For instance, the breakpoint may be any symbol defined for local functions in user's application. This includes statically linked libraries, since they will part of the final executable. On the other hand, a breakpoint cannot be defined for those functions from external (dynamic) libraries.

The program uses de `libdwarf` to retrieve the function's address from DWARF format and then set the breakpoint. Thus, the user's program must be compiled with the '-g' flag. `libelf` is also necessary. They were installed from Linux Mint's repository (version 17.3 'Rosa'. Cinammon 64-bit).

In its standard behavior, it prints only `heap` and `stack` segments.

Memlay was not extensively tested.

## Progress and current features

Only one breakpoint may be determined.

Flag `-v` enables verbose mode.

Flag `-a` enables printing of text, data and bss sections layouts as well.

## Known issues

Program does not handle functions with `DW_AT_inline` attributes properly.

## Usage
`memlay [-v|-a] <breakpoint> <executable> <args>`
