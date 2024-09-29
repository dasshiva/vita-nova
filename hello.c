struct module;
extern int module_depends(const char* d[]);
extern struct module* get_module(const char* name);
extern void* resolve(struct module* h, const char* name);

const char* dependencies[] = { "random", 0 };

int module_main(struct module* h) {
	module_depends(dependencies);
	struct module* random = get_module("random");
	long (*rand)(int) = resolve(random, "randint");
	return 0;
}
