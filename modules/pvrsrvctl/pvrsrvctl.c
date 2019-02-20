#include <stdlib.h>
#include <dlfcn.h>

int main(int argc, char **argv)
{
	void *handle;
	int (*pvr_srv_init) (int);

	handle = dlopen("libsrv_init.so", RTLD_LAZY);
	if(!handle) {
		printf("failed to open libsrv_init.so: %s", dlerror());
		return EXIT_FAILURE;
	}
	pvr_srv_init  = dlsym(handle, "SrvInit");
	if(!pvr_srv_init) {
		printf("failed to get symbol SrvInit: %s", dlerror());
		return EXIT_FAILURE;
	}
	(*pvr_srv_init)(1);

	return EXIT_SUCCESS;
}
