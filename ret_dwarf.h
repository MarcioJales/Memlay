#include <stdlib.h>
#include <stdio.h>
#include <dwarf.h>
#include <libdwarf.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include "colors.h"

Dwarf_Addr dwarfAnalysis(char* path, char* symbol);
