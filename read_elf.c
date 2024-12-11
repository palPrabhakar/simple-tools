#include <stdio.h>
#include <elf.h>

void read_header(FILE *pfile, Elf64_Ehdr *hdr) {
    printf("\n");

    fread(hdr, sizeof(*hdr), 1, pfile);

    printf("e_indent: %s\n", hdr->e_ident);

    printf("e_type: %hu\n", hdr->e_type);
    if(hdr->e_type != ET_EXEC) {
        printf("Error Unknown elf type.\n");
    }

    printf("e_machine: %hu\n", hdr->e_machine);
    if(hdr->e_machine != EM_X86_64) {
        printf("Error Unknown machine type.\n");
    }

    printf("e_version: %u\n", hdr->e_version);
    if(hdr->e_version == EV_NONE) {
        printf("Error invalid elf version number.\n");
    }

    printf("e_entry: 0x%lx\n", hdr->e_entry);

    printf("e_phoff: %lu\n", hdr->e_phoff);
    printf("e_shoff: %lu\n", hdr->e_shoff);
    printf("e_flags: %u\n", hdr->e_flags);
    printf("e_ehsize: %hu\n", hdr->e_ehsize);
    printf("e_phentsize: %hu\n", hdr->e_phentsize);
    printf("e_phnum: %hu\n", hdr->e_phnum);
    printf("e_shentsize: %hu\n", hdr->e_shentsize);
    printf("e_shnum: %hu\n", hdr->e_shnum);
    printf("e_shstrndx: %hu\n", hdr->e_shstrndx);
}

void read_program_header(FILE *pfile, size_t offset, size_t num_headers) {
    printf("\n");
    printf("Reading program header\n");
    Elf64_Phdr program_hdr[num_headers];
    fseek(pfile, offset, SEEK_SET);
    fread(&program_hdr, sizeof(program_hdr[0]), num_headers, pfile);

    for(size_t i = 0; i < num_headers; ++i) {
        printf("\n");
        printf("p_type: 0x%x\n", program_hdr[i].p_type);
        printf("p_flags: %u\n", program_hdr[i].p_flags);
        printf("p_offset: %lu\n", program_hdr[i].p_offset);
        printf("p_vaddr: 0x%lx\n", program_hdr[i].p_vaddr);
        printf("p_paddr: 0x%lx\n", program_hdr[i].p_paddr);
        printf("p_filesz: %lu\n", program_hdr[i].p_filesz);
        printf("p_memsz: %lu\n", program_hdr[i].p_memsz);
        printf("p_align: %lu\n", program_hdr[i].p_align);
        printf("\n");
    }
}

void read_section_header(FILE *pfile, size_t offset, size_t num_sections) {
    printf("\n");
    printf("Reading program header\n");
    Elf64_Shdr section_hdr[num_sections];
    fseek(pfile, offset, SEEK_SET);
    fread(&section_hdr, sizeof(section_hdr[0]), num_sections, pfile);

    for(size_t i = 0; i < num_sections; ++i) {
        printf("\n");
        printf("sh_name: %u\n", section_hdr[i].sh_name);
        printf("sh_type: %u\n", section_hdr[i].sh_type);
        printf("sh_flags: %lu\n", section_hdr[i].sh_flags);
        printf("sh_addr: 0x%lx\n", section_hdr[i].sh_addr);
        printf("sh_offset: %lu\n", section_hdr[i].sh_offset);
        printf("sh_size: %lu\n", section_hdr[i].sh_size);
        printf("sh_link: %u\n", section_hdr[i].sh_link);
        printf("sh_info: %u\n", section_hdr[i].sh_info);
        printf("sh_addralign: %lu\n", section_hdr[i].sh_addralign);
        printf("sh_entsize: %lu\n", section_hdr[i].sh_entsize);
        printf("\n");
    }
}

void read_elf(const char *file_name) {
    FILE *pfile;
    pfile = fopen(file_name, "rb");

    if(pfile == NULL) {
        printf("Unable to open file %s\n", file_name);
    }

    Elf64_Ehdr hdr;
    read_header(pfile, &hdr);
    read_program_header(pfile, hdr.e_phoff, hdr.e_phnum);
    read_section_header(pfile, hdr.e_shoff, hdr.e_shnum);
    fclose(pfile);
}
