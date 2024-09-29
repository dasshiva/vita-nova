#include <elf.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdint.h>

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
		warn("Not an ELF file, check for typos or any other mistakes in the file");
		return -1;
	}
	// This is an ELF file
	if (ehdr->e_ident[EI_CLASS] != ELFCLASS64) {
		warn("Only 64 bit modules can be loaded");
		return -1;
	}
	// This is a 64-bit ELF file
	if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
		warn("Only little endian data is allowed in ELF file, maybe this architecture is unsupported");
		return -1;
	}
	// This is a 64-bit little endian ELF file
	if (ehdr->e_ident[EI_VERSION] != EV_CURRENT) {
		warn("Invalid elf version");
		return -1;
	}

	if (ehdr->e_ident[EI_OSABI] != ELFOSABI_SYSV) {
		warn("Only System V ABI ELF files are allowed");
		return -1;
	}
	
	if (ehdr->e_type != ET_REL) {
		warn("Only relocatable files are allowed as modules");
		return -1;
	}

#ifdef __x86_64__
	if (ehdr->e_machine != EM_X86_64) {
#elif  __aarch64__
	if (ehdr->e_machine != EM_AARCH64) {
#endif
		warn("Unsupported ELF architecture");
		return -1;
	}

	// file is guaranteed to be an ELF file right now, 
	// though we cannot assume it is perfectly in format
	// as it may have malformed section headers or relocations we don't know of
	

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
