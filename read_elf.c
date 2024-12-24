#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "read_elf.h"

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

void read_symbol_table(FILE *pfile, size_t idx_symtab, size_t idx_strtab,
                       Elf64_Shdr section_header[]) {
  Elf64_Shdr symtab = section_header[idx_symtab];
  Elf64_Shdr strtab = section_header[idx_strtab];

  Elf64_Sym *symbols = malloc(symtab.sh_size);
  fseek(pfile, symtab.sh_offset, SEEK_SET);
  fread(symbols, symtab.sh_size, 1, pfile);

  // Read the string table
  char *strtab_data = malloc(strtab.sh_size);
  fseek(pfile, strtab.sh_offset, SEEK_SET);
  fread(strtab_data, strtab.sh_size, 1, pfile);

  int num_symbols = symtab.sh_size / symtab.sh_entsize;
  for (int i = 0; i < num_symbols; i++) {
    Elf64_Sym sym = symbols[i];
    const char *name = &strtab_data[sym.st_name]; // Symbol name

    printf("Symbol %d:\n", i);
    printf("  Name: %s\n", name);
    printf("  Value: 0x%lx\n", sym.st_value);
    printf("  Size: %lu\n", sym.st_size);
    printf("  Info: 0x%x\n", sym.st_info);
    printf("  Section Index: %d\n", sym.st_shndx);
  }
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

  return 0;
}

symbol_t *get_elf_symbols(const char *file_name) {
  FILE *pfile;
  pfile = fopen(file_name, "rb");
  if (pfile == NULL) {
    fprintf(stderr, "Unable to open file %s\n", file_name);
    return NULL;
  }

  Elf64_Ehdr hdr;
  if (read_header(pfile, &hdr)) {
    fclose(pfile);
    return NULL;
  }

  Elf64_Shdr shdr[hdr.e_shnum];
  if (read_section_header(pfile, &hdr, (Elf64_Shdr *)&shdr)) {
    fclose(pfile);
    return NULL;
  }

  size_t idx_symtab;
  size_t idx_strtab;
  get_symtab_strtab_idx(pfile, &hdr, (Elf64_Shdr *)&shdr, &idx_symtab,
                        &idx_strtab);

  printf("idx_symtab: %lu, idx_strtab: %lu\n", idx_symtab, idx_strtab);
  // read_symbol_table(pfile, idx_symtab, idx_strtab, section_hdr);

  fclose(pfile);

  return NULL;
}
