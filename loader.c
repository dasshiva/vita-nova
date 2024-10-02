#include <elf.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdint.h>
#include <string.h>

#define VERSION "0.0.1"
#define error(...) { printf(__VA_ARGS__); perror("Cause"); exit(1); }
#define warn(...) { printf(__VA_ARGS__); }
void print_usage() {
	static const char* help = 
		"Usage : loader [module]\n"
		"[module] is the name of the vita-nova object to be loaded\n"
		"Specifying absolute and relative paths are both allowed\n"
		"The modules will be linked, loaded after their respective module_main() functions are run\n"
		"Module loader version " VERSION "\n";
	printf("%s", help);
	exit(0);

}

int parse_elf(uint8_t* file) {
	Elf64_Ehdr* ehdr = (Elf64_Ehdr*) file;
	// May not be an ELF file
	if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
		ehdr->e_ident[EI_MAG2] != ELFMAG2 || ehdr->e_ident[EI_MAG3] != ELFMAG3) {
		warn("Not an ELF file, check for typos or any other mistakes in the file\n");
		return -1;
	}

	// This is an ELF file
	if (ehdr->e_ident[EI_CLASS] != ELFCLASS64 || ehdr->e_ehsize != sizeof(Elf64_Ehdr) 
			|| ehdr->e_shentsize != sizeof(Elf64_Ehdr)) {
		warn("Only 64 bit modules can be loaded\n");
		return -1;
	}

	// This is a 64-bit ELF file
	if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
		warn("Only little endian data is allowed in ELF file, maybe this architecture is unsupported\n");
		return -1;
	}

	// This is a 64-bit little endian ELF file
	if (ehdr->e_ident[EI_VERSION] != EV_CURRENT || ehdr->e_version != EV_CURRENT) {
		warn("Invalid elf version\n");
		return -1;
	}

	// Follows the System V ABI
	if (ehdr->e_ident[EI_OSABI] != ELFOSABI_SYSV) {
		warn("Only System V ABI ELF files are allowed\n");
		return -1;
	}
	
	// File is relocatable
	if (ehdr->e_type != ET_REL) {
		warn("Only relocatable files are allowed as modules\n");
		return -1;
	}

#ifdef __x86_64__
	if (ehdr->e_machine != EM_X86_64) {
#elif  __aarch64__
	if (ehdr->e_machine != EM_AARCH64) {
#endif
		warn("Unsupported ELF architecture\n");
		return -1;
	}

	if (ehdr->e_entry != 0) {
		warn("ELF file must not have an entry\n");
		return -1;
	}

	if (ehdr->e_shnum == 0) {
		warn("No section headers in file\n");
		return -1;
	}

	if (ehdr->e_shstrndx == 0) {
		warn("No section header string table found\n");
		return -1;
	}

	Elf64_Shdr* shdr = (Elf64_Shdr*) (file + ehdr->e_shoff);
	Elf64_Shdr* strtab = shdr + ehdr->e_shstrndx;
	const char* section_names = (char*)(file + strtab->sh_offset);
	
	Elf64_Shdr *text = NULL, *rela_text = NULL, *symtab = NULL;
	for (uint64_t i = 1; i <= ehdr->e_shnum; i++) { // We skip the first index as it is a null entry
		Elf64_Shdr* t = shdr + i;
		if (!strcmp(section_names + t->sh_name, ".text")) // strcmp returns 0 if true
			text = t;
		else if (!strcmp(section_names + t->sh_name, ".rela.text"))
			rela_text = t;
		else if (!strcmp(section_names + t->sh_name, ".symtab"))
			symtab = t;
	}

	if (!text || !rela_text || !symtab) {
		warn("Object file does not have required section headers\n");
		return -1;
	}
	
	uint8_t* module_code = mmap(NULL, text->sh_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	
	if (module_code == MAP_FAILED) 
		error("Failed to allocate memory for module code\n");
	memcpy(module_code, file + text->sh_offset, text->sh_size);

	return 0;
}

int main(int argc, const char* argv[]) {
	if (argc == 1)
		print_usage();
	int fd = open(argv[1], O_RDONLY);
	if (fd == -1) 
		error("Failed to open module %s\n", argv[1]);
	struct stat st;
	if (fstat(fd, &st) == -1) 
		error("Failed to stat module %s\n", argv[1]);
	uint8_t* file = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (file == MAP_FAILED)
		error("Could not map module %s to memory", argv[1]);
	parse_elf(file);
	return 0;
}
