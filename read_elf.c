#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "read_elf.h"

static symbols_t symbols;

const char *find_name(uint64_t addr) {
  for (size_t i = 0; i < symbols.len - 1; ++i) {
    if(symbols.symbols[i].address == 0x0)
      continue;

    if(symbols.symbols[i].address == symbols.symbols[i+1].address)
      continue;

    if(symbols.symbols[i+1].address > addr)
      return symbols.symbols[i].symbol_name;
  }
  return symbols.symbols[symbols.len - 1].symbol_name;
}

int compare(const void *a, const void *b) {
  symbol_t arg1 = *(const symbol_t *)a;
  symbol_t arg2 = *(const symbol_t *)b;

  if (arg1.address < arg2.address)
    return -1;
  if (arg1.address > arg2.address)
    return 1;
  return 0;
}

int read_header(FILE *pfile, Elf64_Ehdr *hdr) {
  fseek(pfile, 0, SEEK_SET);
  if (fread(hdr, sizeof(*hdr), 1, pfile) != 1) {
    fprintf(stderr, "Unable to read elf headers\n");
    return 1;
  }

  if (hdr->e_type != ET_EXEC) {
    fprintf(stderr, "Error Unknown elf type.\n");
    return 1;
  }

  if (hdr->e_machine != EM_X86_64) {
    fprintf(stderr, "Error Unknown machine type.\n");
    return 1;
  }

  return 0;
}

symbol_t *read_symbol_table(FILE *pfile, size_t idx_symtab, size_t idx_strtab,
                            Elf64_Shdr *section_header, size_t *num_symbols) {
  Elf64_Shdr symtab = section_header[idx_symtab];
  Elf64_Shdr strtab = section_header[idx_strtab];

  Elf64_Sym *symbols = malloc(symtab.sh_size);
  fseek(pfile, symtab.sh_offset, SEEK_SET);
  if (fread(symbols, symtab.sh_size, 1, pfile) != 1) {
    fprintf(stderr, "Unable to read the symbol table\n");
    free(symbols);
    return NULL;
  }

  // Read the string table
  char *strtab_data = malloc(strtab.sh_size);
  fseek(pfile, strtab.sh_offset, SEEK_SET);
  if (fread(strtab_data, strtab.sh_size, 1, pfile) != 1) {
    fprintf(stderr, "Unable to read the string table\n");
    free(strtab_data);
    return NULL;
  }

  *num_symbols = symtab.sh_size / symtab.sh_entsize;

  symbol_t *symbol_arr = malloc(sizeof(symbol_t) * *num_symbols);

  for (size_t i = 0; i < *num_symbols; i++) {
    Elf64_Sym sym = symbols[i];
    const char *name = &strtab_data[sym.st_name]; // Symbol name
    size_t len = strlen(name);
    symbol_arr[i].symbol_name = malloc(sizeof(char) * len);
    memcpy((void *)symbol_arr[i].symbol_name, name, len);
    symbol_arr[i].address = sym.st_value;
  }

  free(symbols);
  free(strtab_data);

  return symbol_arr;
}

int read_section_header(FILE *pfile, Elf64_Ehdr *hdr, Elf64_Shdr *shdr) {
  fseek(pfile, hdr->e_shoff, SEEK_SET);
  if (fread(shdr, sizeof(*shdr), hdr->e_shnum, pfile) != hdr->e_shnum) {
    fprintf(stderr, "Unable to read section headers\n");
    return 1;
  }

  return 0;
}

int get_symtab_strtab_idx(FILE *pfile, Elf64_Ehdr *hdr, Elf64_Shdr *shdr,
                          size_t *idx_symtab, size_t *idx_strtab) {
  fseek(pfile, shdr[hdr->e_shstrndx].sh_offset, SEEK_SET);
  char *shstrtab = malloc(shdr[hdr->e_shstrndx].sh_size);
  if (fread(shstrtab, 1, shdr[hdr->e_shstrndx].sh_size, pfile) !=
      shdr[hdr->e_shstrndx].sh_size) {
    fprintf(stderr, "Unable to read the section header string table\n");
    free(shstrtab);
    return 1;
  }

  for (size_t i = 0; i < hdr->e_shnum; ++i) {
    if (shdr[i].sh_type == SHT_SYMTAB &&
        strcmp(&shstrtab[shdr[i].sh_name], ".symtab") == 0) {
      *idx_symtab = i;
    }
    if (shdr[i].sh_type == SHT_STRTAB &&
        strcmp(&shstrtab[shdr[i].sh_name], ".strtab") == 0) {
      *idx_strtab = i;
    }
  }

  free(shstrtab);

  return 0;
}

void init_stack_tracer(const char *file_name) {
  // symbols_t result = {.len = 0, .symbols = NULL};
  FILE *pfile;
  pfile = fopen(file_name, "rb");
  if (pfile == NULL) {
    fprintf(stderr, "Unable to open file %s\n", file_name);
  }

  Elf64_Ehdr hdr;
  if (read_header(pfile, &hdr)) {
    fclose(pfile);
  }

  Elf64_Shdr shdr[hdr.e_shnum];
  if (read_section_header(pfile, &hdr, shdr)) {
    fclose(pfile);
  }

  size_t idx_symtab = -1;
  size_t idx_strtab = -1;
  get_symtab_strtab_idx(pfile, &hdr, shdr, &idx_symtab, &idx_strtab);

  if(idx_symtab == -1) {
      fprintf(stderr, "Unable to find the symbol table index\n");
      return;
  }
  if(idx_strtab == -1) {
      fprintf(stderr, "Unable to find the string table index\n");
      return;
  }

  symbols.symbols =
      read_symbol_table(pfile, idx_symtab, idx_strtab, shdr, &symbols.len);

  qsort(symbols.symbols, symbols.len, sizeof(symbol_t), compare);

  fclose(pfile);
}
