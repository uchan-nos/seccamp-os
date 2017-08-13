// 2016 uchan
#ifndef ELF_H_
#define ELF_H_
#include <stdint.h>

#define EI_NIDENT 16

typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Word;
typedef int32_t Elf32_Sword;
typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Off;
typedef uint32_t Elf32_Size;

typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef int32_t Elf64_Sword;
typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint64_t Elf64_Size;
typedef uint64_t Elf64_Xword;
typedef int64_t Elf64_Sxword;

typedef struct {
	unsigned char e_ident[EI_NIDENT];
	Elf32_Half e_type;
	Elf32_Half e_machine;
	Elf32_Word e_version;
	Elf32_Addr e_entry;
	Elf32_Off  e_phoff;
	Elf32_Off  e_shoff;
	Elf32_Word e_flags;
	Elf32_Half e_ehsize;
	Elf32_Half e_phentsize;
	Elf32_Half e_phnum;
	Elf32_Half e_shentsize;
	Elf32_Half e_shnum;
	Elf32_Half e_shstrndx;
} Elf32_Ehdr;

typedef struct {
	unsigned char e_ident[EI_NIDENT];
	Elf64_Half e_type;
	Elf64_Half e_machine;
	Elf64_Word e_version;
	Elf64_Addr e_entry;
	Elf64_Off  e_phoff;
	Elf64_Off  e_shoff;
	Elf64_Word e_flags;
	Elf64_Half e_ehsize;
	Elf64_Half e_phentsize;
	Elf64_Half e_phnum;
	Elf64_Half e_shentsize;
	Elf64_Half e_shnum;
	Elf64_Half e_shstrndx;
} Elf64_Ehdr;

#define IS_ELF(ehdr) \
	((ehdr).e_ident[0] == 0x7f && \
	 (ehdr).e_ident[1] == 'E' && \
	 (ehdr).e_ident[2] == 'L' && \
	 (ehdr).e_ident[3] == 'F')

typedef struct {
	Elf32_Word sh_name;
	Elf32_Word sh_type;
	Elf32_Word sh_flags;
	Elf32_Addr sh_addr;
	Elf32_Off  sh_offset;
	Elf32_Size sh_size;
	Elf32_Word sh_link;
	Elf32_Word sh_info;
	Elf32_Size sh_addralign;
	Elf32_Size sh_entsize;
} Elf32_Shdr;

typedef struct {
	Elf64_Word sh_name;
	Elf64_Word sh_type;
	Elf64_Xword sh_flags;
	Elf64_Addr sh_addr;
	Elf64_Off  sh_offset;
	Elf64_Size sh_size;
	Elf64_Word sh_link;
	Elf64_Word sh_info;
	Elf64_Size sh_addralign;
	Elf64_Size sh_entsize;
} Elf64_Shdr;

#define SHN_UNDEF 0

#define SHT_NULL          0
#define SHT_PROGBITS      1
#define SHT_SYMTAB        2
#define SHT_STRTAB        3
#define SHT_RELA          4
#define SHT_HASH          5
#define SHT_DYNAMIC       6
#define SHT_NOTE          7
#define SHT_NOBITS        8
#define SHT_REL           9
#define SHT_SHLIB         10
#define SHT_DYNSYM        11
#define SHT_INIT_ARRAY    14
#define SHT_FINI_ARRAY    15
#define SHT_PREINIT_ARRAY 16
#define SHT_GROUP         17
#define SHT_SYMTAB_SHNDX  18
#define SHT_LOOS          0x60000000
#define SHT_HIOS          0x6fffffff
#define SHT_LOPROC        0x70000000
#define SHT_HIPROC        0x7fffffff
#define SHT_LOUSER        0x80000000
#define SHT_HIUSER        0xffffffff

typedef struct {
	Elf32_Word p_type;
	Elf32_Off  p_offset;
	Elf32_Addr p_vaddr;
	Elf32_Addr p_paddr;
	Elf32_Size p_filesz;
	Elf32_Size p_memsz;
	Elf32_Word p_flags;
	Elf32_Size p_align;
} Elf32_Phdr;

typedef struct {
	Elf64_Word p_type;
	Elf64_Word p_flags;
	Elf64_Off  p_offset;
	Elf64_Addr p_vaddr;
	Elf64_Addr p_paddr;
	Elf64_Size p_filesz;
	Elf64_Size p_memsz;
	Elf64_Size p_align;
} Elf64_Phdr;

#define PF_X 1
#define PF_W 2
#define PF_R 4

typedef struct {
	Elf32_Word st_name;
	Elf32_Addr st_value;
	Elf32_Size st_size;
	unsigned char st_info;
	unsigned char st_other;
	Elf32_Half st_shndx;
} Elf32_Sym;

typedef struct {
	Elf64_Word st_name;
	unsigned char st_info;
	unsigned char st_other;
	Elf64_Half st_shndx;
	Elf64_Addr st_value;
	Elf64_Size st_size;
} Elf64_Sym;

typedef struct {
	Elf32_Addr r_offset;
	Elf32_Word r_info;
} Elf32_Rel;

typedef struct {
	Elf64_Addr r_offset;
	Elf64_Xword r_info;
} Elf64_Rel;

typedef struct {
	Elf32_Addr r_offset;
	Elf32_Word r_info;
	Elf32_Sword r_addend;
} Elf32_Rela;

typedef struct {
	Elf64_Addr r_offset;
	Elf64_Xword r_info;
	Elf64_Sxword r_addend;
} Elf64_Rela;

#define ELF32_R_SYM(r_info) (((r_info) >> 8u)
#define ELF32_R_TYPE(r_info) ((unsigned char)(r_info))

#define ELF64_R_SYM(r_info) ((r_info) >> 32)
#define ELF64_R_TYPE(r_info) ((r_info) & 0xffffffffL)

#define R_386_NONE 0
#define R_386_32 1
#define R_386_PC32 2

#define R_X86_64_RELATIVE 8

// Utilities

#define ELF32_GET_SHDR(ehdr) ((Elf32_Shdr*)((char*)(ehdr) + (ehdr)->e_shoff))
#define ELF32_GET_PHDR(ehdr) ((Elf32_Phdr*)((char*)(ehdr) + (ehdr)->e_phoff))
#define ELF32_GET_SHSTRTAB(ehdr) ((char*)(ehdr) + ELF32_GET_SHDR(ehdr)[(ehdr)->e_shstrndx].sh_offset)

#define ELF64_GET_SHDR(ehdr) ((Elf64_Shdr*)((char*)(ehdr) + (ehdr)->e_shoff))
#define ELF64_GET_PHDR(ehdr) ((Elf64_Phdr*)((char*)(ehdr) + (ehdr)->e_phoff))
#define ELF64_GET_SHSTRTAB(ehdr) ((char*)(ehdr) + ELF64_GET_SHDR(ehdr)[(ehdr)->e_shstrndx].sh_offset)

Elf32_Shdr* elf32_find_section(Elf32_Ehdr* ehdr, const char* secname);
Elf64_Shdr* elf64_find_section(Elf64_Ehdr* ehdr, const char* secname);

#endif // ELF_H_
