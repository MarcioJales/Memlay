#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "ret_dwarf.h"

Dwarf_Addr getDie(Dwarf_Debug dbg_info, char *symbol)
{
  Dwarf_Unsigned *next_compilation_unit_hdr;
  Dwarf_Bool is_info = 1;
  int ret;

  while(1)
  {
    Dwarf_Die no_die = 0, compilation_unit_die;

    ret = dwarf_next_cu_header_c(dbg_info, is_info, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, next_compilation_unit_hdr, NULL);
    if(ret == DW_DLV_ERROR) {
      fprintf(stderr, RED "Error in dwarf_next_cu_header\n" CLEAR);
      exit(EXIT_FAILURE);
    }
    else if(ret == DW_DLV_NO_ENTRY)
      return 0;

    /* ret == DW_DLV_OK */
    ret = dwarf_siblingof_b(dbg_info, no_die, is_info, &compilation_unit_die, NULL);
    if(ret == DW_DLV_ERROR) {
      fprintf(stderr, RED "Error in dwarf_siblingof_b\n" CLEAR);
      exit(EXIT_FAILURE);
    }
    else if(ret == DW_DLV_NO_ENTRY) { /* Impossible case */
      fprintf(stderr, RED "No DIE of Compilation Unit\n" CLEAR);
      exit(EXIT_FAILURE);
    }
  }
}

Dwarf_Addr dwarfAnalysis(char* path, char* symbol)
{
  Dwarf_Debug dbg_info = 0;
  Dwarf_Addr brkpoint;
  int fd;

  fd = open(path, O_RDONLY);
  if(fd == -1) {
    fprintf(stderr, RED "Failed to open: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }

  if(dwarf_init(fd, DW_DLC_READ, 0, 0, &dbg_info, NULL) != DW_DLV_OK) {
    fprintf(stderr, RED "Failed to initialize DWARF\n" CLEAR);
    exit(EXIT_FAILURE);
  }

  brkpoint = getDie(dbg_info, symbol);

  if(dwarf_finish(dbg_info, NULL) != DW_DLV_OK) {
    fprintf(stderr, RED "Failed to finish DWARF\n" CLEAR);
    exit(EXIT_FAILURE);
  }

  if(close(fd) == -1) {
    fprintf(stderr, RED "Failed to close: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }

  return brkpoint;
}
