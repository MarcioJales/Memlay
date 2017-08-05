#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>


#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define CLEAR   "\x1b[0m"
#define YELLOW  "\x1b[33m"


void* mapMemory(char* path, int* fd)
{
  struct stat file_info;
  void* mem_map;

  printf(YELLOW "Mapping memory...\n" CLEAR);
  
  *fd = open(path, O_RDONLY);

  if(*fd == -1) {
    fprintf(stderr, RED "Failed to open: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }

  if(fstat(*fd, &file_info) == -1) {
    fprintf(stderr, RED "Failed to get file information: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }

  mem_map = mmap(NULL, file_info.st_size, PROT_READ, MAP_PRIVATE, *fd, 0);

  if(mem_map == MAP_FAILED) {
    fprintf(stderr, RED "Failed to map memmory: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }

  if(close(*fd) == -1) {
    fprintf(stderr, RED "Failed to close: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }
  
  return mem_map;  
};


Elf64_Addr lookupSymbol(Elf64_Ehdr* elf_hdr, Elf64_Shdr* section_hdr, char* symbol, uint8_t* file_begin)
{
  int count;
  Elf64_Sym* sym_table;
  char* str_table;
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
	if(strcmp(&str_table[sym_table->st_name], symbol) == 0)
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


char** parseArgs(char** argv, int argc)
{
  char** args;
  int num_args = argc - 1;
  int count = 0;

  printf(YELLOW "Parsing arguments...\n" CLEAR);
  
  args = (char**) malloc(num_args*sizeof(char*));
  
  while(count < num_args) {
    if(count != num_args-1) {
      args[count] = strdup(argv[count+2]);
      
      if(args[count] == NULL) {
	fprintf(stderr, RED "Failed to strdup: %s\n" CLEAR, strerror(errno));
	exit(EXIT_FAILURE);
      }	
    }
    else
      args[count] = NULL;
    count++;
  }

  return args;
};


int main(int argc, char** argv)
{
  int fd, status;
  pid_t pid_child;
  Elf64_Addr sym_addr;
  char* sym_name = (char*) malloc(strlen(argv[1]));
  char* prog_path = (char*) malloc(strlen(argv[2]));
  char** args_parsed; 
  void* mapped_mem;

  strcpy(prog_path, argv[2]);
  mapped_mem = mapMemory(prog_path, &fd);
  
  strcpy(sym_name, argv[1]);
  sym_addr = checkBinary((uint8_t*) mapped_mem, sym_name);
  if(sym_addr == 0) {
    fprintf(stderr, RED "Error: " CLEAR "symbol not found\n");
    exit(EXIT_FAILURE);
  }
  else
    printf(GREEN "Symbol found\n" CLEAR);
  
  args_parsed = parseArgs(argv, argc);
  
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
    if(execv(prog_path, args_parsed) == -1) {
      fprintf(stderr, RED "Failed to start the tracee: %s\n" CLEAR, strerror(errno));
      exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
  }

  wait(&status);
  printf(YELLOW "Beginning of the analysis of process %d (%s), for the breakpoint at %lx\n" CLEAR, pid_child, prog_path, sym_addr);
  
  return 0;
}
