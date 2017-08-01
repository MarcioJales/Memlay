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

void* mapMemory(char* path, int* fd)
{
  struct stat file_info;
  void* mem_map;

  printf(GREEN "Mapping memory...\n" CLEAR);
  
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
    fprintf(stderr, "Failed to close: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  
  return mem_map;  
};

void checkBinary(uint8_t* map)
{
  printf(GREEN "Checking binary file...\n" CLEAR);
  
  if(map[0] != 0x7f || strncmp((char*) map+1, "ELF", 3)) {
    printf(RED "Error: " CLEAR "the file specified is not a ELF binary.\n");
    exit(EXIT_FAILURE);
  }
}

char** parseArgs(char** argv_l, int argc_l)
{
  char** args;
  int num_args = argc_l - 1;
  int count = 0;

  printf(GREEN "Parsing arguments...\n" CLEAR);
  
  args = (char**) malloc(num_args*sizeof(char*));
  
  while(count < num_args) {
    if(count != num_args-1) {
      args[count] = strdup(argv_l[count+2]);
      
      if(args[count] == NULL) {
	fprintf(stderr, "Failed to strdup: %s\n", strerror(errno));
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
  int fd;
  pid_t pid_child;
  Elf64_Addr sym_addr;
  char* sym_name;
  char** args_parsed; 
  void* mapped_mem;
  Elf64_Ehdr* elf_hdr; 
  Elf64_Phdr* program_hdr;
  Elf64_Shdr* section_hdr;

  mapped_mem = mapMemory(argv[2], &fd);
  checkBinary((uint8_t*) mapped_mem);
  args_parsed = parseArgs(argv, argc);
  
  pid_child = fork();
  
  if(pid_child == -1) {
    fprintf(stderr, "Failed to fork: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  else if (pid_child == 0){
    
  }
  else {
  }
  
  return 0;
}
