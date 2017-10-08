#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <fcntl.h>
#include <libgen.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>


#define RED                "\x1b[31m"
#define GREEN              "\x1b[32m"
#define CLEAR              "\x1b[0m"
#define YELLOW             "\x1b[33m"

#define BUF_LENGTH          256

#define TEXT                0
#define DATA                1
#define BSS                 2
#define HEAP                3
#define STACK               4


void* mapMemory(char* path)
{
  struct stat file_info;
  int fd;
  void* mem_map;

  printf(YELLOW "Mapping memory...\n" CLEAR);

  fd = open(path, O_RDONLY);
  if(fd == -1) {
    fprintf(stderr, RED "Failed to open: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }

  if(fstat(fd, &file_info) == -1) {
    fprintf(stderr, RED "Failed to get file information: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }

  mem_map = mmap(NULL, file_info.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if(mem_map == MAP_FAILED) {
    fprintf(stderr, RED "Failed to map memmory: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }

  if(close(fd) == -1) {
    fprintf(stderr, RED "Failed to close: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }

  return mem_map;
};




Elf64_Addr lookupSymbol(Elf64_Ehdr *elf_hdr, Elf64_Shdr *section_hdr, char *symbol, uint8_t *file_begin)
{
  int count;
  Elf64_Sym *sym_table;
  char *str_table;
  int str_index;

  /* section_hdr[count] = beginning of section header
   * section_hdr[count].sh_offset = beginning of section
   * sh_link when sh_type = SHT_SYMTAB means that it holds the index to the string table section
   */
  for(count = 0; count < elf_hdr->e_shnum; count++) {
    if(section_hdr[count].sh_type == SHT_SYMTAB) {
      sym_table = (Elf64_Sym*) (file_begin + section_hdr[count].sh_offset);

      str_index = section_hdr[count].sh_link;
      str_table = (char*) (file_begin + section_hdr[str_index].sh_offset);

      /* Iterate over the symbol tables in the .symtab section
       * st_value holds the entry point (address) of the function represented by the symbol
       */
      int j;
      for(j = 0; j < section_hdr[count].sh_size/sizeof(Elf64_Sym); j++) {
        if(strncmp(&str_table[sym_table->st_name], symbol, strlen(symbol)) == 0)
          return sym_table->st_value;
        sym_table++;
      }
    }
  }
  return 0;
};




Elf64_Addr checkBinary(uint8_t* map, char* symbol)
{
  Elf64_Ehdr* elf_hdr;
  Elf64_Shdr* section_hdr;

  printf(YELLOW "Checking binary file...\n" CLEAR);

  if(map[0] != 0x7f || strncmp((char*) map+1, "ELF", 3)) {
    printf(RED "Error: " CLEAR "the file specified is not a ELF binary.\n");
    exit(EXIT_FAILURE);
  }

  elf_hdr = (Elf64_Ehdr*) map;
  if(elf_hdr->e_type != ET_EXEC) {
    printf(RED "Error: " CLEAR "the file specified is not a ELF executable.\n");
    exit(EXIT_FAILURE);
  }
  if(elf_hdr->e_shoff == 0 || elf_hdr->e_shnum == 0 || elf_hdr->e_shstrndx == SHN_UNDEF) {
    printf(RED "Error: " CLEAR "section header table or section name string table were not found.\n");
    exit(EXIT_FAILURE);
  }
  else
    section_hdr = (Elf64_Shdr*)(map + elf_hdr->e_shoff);

  return lookupSymbol(elf_hdr, section_hdr, symbol, map);
};




void getSection(FILE *fs, char *readbuf, uint8_t section)
{
  char *start_addr, *end_addr;
  int offset;

  if(section == TEXT)
    printf(GREEN "[Text section]\n" CLEAR);
  if(section == DATA)
    printf(GREEN "[Data section]\n" CLEAR);
  if(section == BSS)
    printf(GREEN "[Bss section]\n" CLEAR);
  if(section == HEAP)
    printf(GREEN "[Heap]\n" CLEAR);
  if(section == STACK)
    printf(GREEN "[Stack]\n" CLEAR);

  offset = ftell(fs);

  fgets(readbuf, BUF_LENGTH, fs);
  if(section == HEAP) {
    if(strncmp(readbuf + 74, "heap", 4)) {
      printf(RED "None\n" CLEAR);
	    fseek(fs, offset, SEEK_SET);
      return;
    }
  }
  if(section == STACK) {
    while(strncmp(readbuf + 74, "stack", 5)) {
      offset = ftell(fs);
      fgets(readbuf, BUF_LENGTH, fs);
    }
    fseek(fs, offset, SEEK_SET);
  }

  start_addr = strtok(readbuf, "-");
  printf(GREEN "%s" CLEAR, start_addr);

  fseek(fs, offset + strlen(start_addr), SEEK_SET);
  fgets(readbuf, BUF_LENGTH, fs);
  end_addr = strtok(readbuf, " ");
  printf(GREEN "%s\n" CLEAR, end_addr);
};




void parseMapsFile(char *maps_path, char *prog_path)
{
  FILE *fs;
  uint8_t section;
  char readbuf[BUF_LENGTH];
  char *basename_prog;

  fs = fopen(maps_path, "r");
  if(fs == NULL) {
    fprintf(stderr, RED "Failed to open: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }

  /* 73 is the offset to get to the pathname of the file */
  if(fseek(fs, 73, SEEK_SET) == -1) {
    fprintf(stderr, RED "Failed to operate on the file: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }
  fgets(readbuf, BUF_LENGTH, fs);
  rewind(fs);

  basename_prog = basename(prog_path);
  if(!strncmp(basename_prog, basename(readbuf), strlen(basename_prog))) {
    for(section = 0; section < 5; section++)
      getSection(fs, readbuf, section);
  }
  else
    fprintf(stderr, RED "File described in \"maps\" do not match the tracee\n" CLEAR);

  fclose(fs);
};




void printLayout(pid_t pid, char* prog_path)
{
  char* maps_path = (char*) malloc(16); /* 'pid' may have at most 5 digits */

  printf("\n\n");
  printf(YELLOW "\nPrinting the layout:\n" CLEAR);
  if(sprintf(maps_path, "/proc/%d/maps", pid) < 0)
    fprintf(stderr, RED "Error to parse the path to \"maps\"\n" CLEAR);

  parseMapsFile(maps_path, prog_path);
  printf("\n\n");
};




void adjustRegisters(pid_t pid, Elf64_Addr sym_addr, long orig_data, long trap)
{
  struct user_regs_struct registers;

  if(ptrace(PTRACE_GETREGS, pid, NULL, &registers) == -1) {
    fprintf(stderr, RED "Failed to get registers information: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }

  if(ptrace(PTRACE_POKEDATA, pid, sym_addr, orig_data) == -1) {
    fprintf(stderr, RED "Failed to poke data: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }

  registers.rip = registers.rip - 1;
  if(ptrace(PTRACE_SETREGS, pid, NULL, &registers) == -1) {
    fprintf(stderr, RED "Failed to get registers information: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }

  if(ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL) == -1) {
    fprintf(stderr, RED "Failed to continue the tracing: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }
  wait(NULL);

  if(ptrace(PTRACE_POKEDATA, pid, sym_addr, trap) == -1) {
    fprintf(stderr, RED "Failed to poke data: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }

};




int main(int argc, char** argv)
{
  int status;
  long orig_data, trap; /* The 'trap' is the opcode '0xcc', which means that, ate that address, the execution must stop */
  pid_t pid_child;
  Elf64_Addr sym_addr;
  void* mapped_mem;

  if(argc < 3) {
    printf("Usage: memlay <breakpoint> <program>\n");
    exit(EXIT_SUCCESS);
  }

  char* sym_name = (char*) malloc(strlen(argv[1]));
  char* prog_path = (char*) malloc(strlen(argv[2]));

  strcpy(prog_path, argv[2]);
  mapped_mem = mapMemory(prog_path);

  strcpy(sym_name, argv[1]);
  sym_addr = checkBinary((uint8_t*) mapped_mem, sym_name);
  if(sym_addr == 0) {
    fprintf(stderr, RED "Error: " CLEAR "symbol not found\n");
    exit(EXIT_FAILURE);
  }
  else
    printf(GREEN "Symbol found\n" CLEAR);
  printf(YELLOW "Breakpoint at %lx\n" CLEAR, sym_addr);

  pid_child = fork();
  if(pid_child == -1) {
    fprintf(stderr, RED "Failed to fork: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }
  if (pid_child == 0) {
    if(ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1) {
      fprintf(stderr, RED "Failed to start tracing: %s\n" CLEAR, strerror(errno));
      exit(EXIT_FAILURE);
    }
    if(execv(prog_path, argv + 2) == -1) {
      fprintf(stderr, RED "Failed to start the tracee: %s\n" CLEAR, strerror(errno));
      exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
  }

  wait(&status);
  printf(YELLOW "Beginning of the analysis of process %d (%s)\n" CLEAR, pid_child, prog_path);

  /* Since  the  value  returned by a successful PTRACE_PEEK* request may be
   * -1, the caller must clear errno before the  call,  and  then  check  it
   * afterward to determine whether or not an error occurred.
   */

  errno = 0;
  orig_data = ptrace(PTRACE_PEEKDATA, pid_child, sym_addr, NULL);
  if(orig_data == -1) {
    if(errno != 0) {
      fprintf(stderr, RED "Failed to peek data: %s\n" CLEAR, strerror(errno));
      exit(EXIT_FAILURE);
    }
  }

  trap = (orig_data & ~0xff) | 0xcc;
  if(ptrace(PTRACE_POKEDATA, pid_child, sym_addr, trap) == -1) {
    fprintf(stderr, RED "Failed to poke data: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }

  while(1) {
    if(ptrace(PTRACE_CONT, pid_child, NULL, NULL) == -1) {
      fprintf(stderr, RED "Failed to continue the tracing: %s\n" CLEAR, strerror(errno));
      exit(EXIT_FAILURE);
    }
    wait(&status);

    if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
      printf(YELLOW "Hit the breakpoint %lx\n" CLEAR, sym_addr);

      printLayout(pid_child, prog_path);
      adjustRegisters(pid_child, sym_addr, orig_data, trap);
    }

    if(WIFEXITED(status)) {
      printf(YELLOW "\nCompleted tracing pid %d\n" CLEAR, pid_child);
      break;
    }
  }

  return 0;
}
