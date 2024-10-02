#define MODULE_NAME(x) const char* module_name = x;
#define DEPENDS_ON(...) const char* module_deps[] = { __VA_ARGS__ , 0};

MODULE_NAME("Simple")
DEPENDS_ON("core", "alloc")

extern void module_register();
int module_main() {
	module_register();
	return 0;
}
