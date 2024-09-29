#include <elf.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

#define VERSION "0.0.1"
#define error(...) { printf(__VA_ARGS__); perror("Cause"); exit(1); }

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

int main(int argc, const char* argv[]) {
	if (argc == 1)
		print_usage();
	int fd = open(argv[1], O_RDONLY);
	if (fd == -1) 
		error("Failed to open module %s\n", argv[1]);
	struct stat st;
	if (fstat(fd, &st) == -1) 
		error("Failed to stat module %s\n", argv[1]);
	Elf64_Ehdr* ehdr = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (ehdr == MAP_FAILED)
		error("Could not map module %s to memory", argv[1]);
	return 0;
}
