#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "retdwarf.h"


static Dwarf_Addr getSubprogAddr(Dwarf_Debug dbg_info, Dwarf_Die subprogram, char *symbol)
{
  char *subprogram_name;
  Dwarf_Attribute* attrs;
  Dwarf_Addr brkpoint_addr;
  Dwarf_Signed attrcount, it;

  if(dwarf_diename(subprogram, &subprogram_name, NULL) != DW_DLV_OK) {
    fprintf(stderr, RED "Could not find the attribute name for the subprogram\n" CLEAR);
    exit(EXIT_FAILURE);
  }

  if(strncmp(subprogram_name, symbol, strlen(subprogram_name)) == 0) {
    if(dwarf_attrlist(subprogram, &attrs, &attrcount, NULL) != DW_DLV_OK) {
      fprintf(stderr, RED "Error in dwarf_attrlist\n" CLEAR);
      exit(EXIT_FAILURE);
    }
    for(it = 0; it < attrcount; it++)
    {
        Dwarf_Half attrcode;

        if(dwarf_whatattr(attrs[it], &attrcode, NULL) != DW_DLV_OK) {
          fprintf(stderr, RED "Error in dwarf_whatattr\n" CLEAR);
          exit(EXIT_FAILURE);
        }

        if(attrcode == DW_AT_low_pc) {
          dwarf_formaddr(attrs[it], &brkpoint_addr, 0);
          return brkpoint_addr;
        }
    }
  }
  else
    return 0;
}


static Dwarf_Addr getSubprogDie(Dwarf_Debug dbg_info, Dwarf_Die current_die, Dwarf_Bool is_info, char *symbol)
{
  Dwarf_Addr brkpoint_addr = 0;
  Dwarf_Die child_die, sibling_die;
  Dwarf_Half tag;
  int ret;

  if(dwarf_tag(current_die, &tag, NULL) != DW_DLV_OK) {
    fprintf(stderr, RED "Error in dwarf_tag\n" CLEAR);
    exit(EXIT_FAILURE);
  }
  if(tag == DW_TAG_subprogram) {
    brkpoint_addr = getSubprogAddr(dbg_info, current_die, symbol);
    if(brkpoint_addr != 0)
      return brkpoint_addr;
  }

  ret = dwarf_child(current_die, &child_die, NULL);
  if(ret == DW_DLV_ERROR) {
    fprintf(stderr, RED "Error in dwarf_child\n" CLEAR);
    exit(EXIT_FAILURE);
  }
  else if(ret == DW_DLV_OK) {
    brkpoint_addr = getSubprogDie(dbg_info, child_die, is_info, symbol);
    dwarf_dealloc(dbg_info, child_die, DW_DLA_DIE);
    if(brkpoint_addr != 0)
      return brkpoint_addr;
  }

  /* DW_DLV_OK and DW_DLV_NO_ENTRY */
  ret = dwarf_siblingof_b(dbg_info, current_die, is_info, &sibling_die, NULL);
  if(ret == DW_DLV_ERROR) {
    fprintf(stderr, RED "Error in dwarf_siblingof_b\n" CLEAR);
    exit(EXIT_FAILURE);
  }
  else if(ret == DW_DLV_OK) {
    brkpoint_addr = getSubprogDie(dbg_info, sibling_die, is_info, symbol);
    dwarf_dealloc(dbg_info, sibling_die, DW_DLA_DIE);
    if(brkpoint_addr != 0)
      return brkpoint_addr;
  }

  /* DW_DLV_NO_ENTRY */
  return 0;
}


static Dwarf_Addr startDwarfAnalysis(Dwarf_Debug dbg_info, char *symbol)
{
  Dwarf_Addr brkpoint_addr = 0;
  Dwarf_Unsigned next_compilation_unit_hdr;
  Dwarf_Bool is_info = 1;
  int ret;

  while(1)
  {
    Dwarf_Die no_die = 0, compilation_unit_die;

    ret = dwarf_next_cu_header_c(dbg_info, is_info, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &next_compilation_unit_hdr, NULL);
    if(ret == DW_DLV_ERROR) {
      fprintf(stderr, RED "Error in dwarf_next_cu_header\n" CLEAR);
      exit(EXIT_FAILURE);
    }
    else if(ret == DW_DLV_NO_ENTRY)
      break;

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

    brkpoint_addr = getSubprogDie(dbg_info, compilation_unit_die, is_info, symbol);
    dwarf_dealloc(dbg_info, compilation_unit_die, DW_DLA_DIE);
  }

  return brkpoint_addr;
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

  brkpoint = startDwarfAnalysis(dbg_info, symbol);

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
