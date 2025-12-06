#include <cstdio>
#include <cstdlib>

#include "meshoptimizer.h"

int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;

	meshopt_generateVertexRemap(NULL, NULL, 0, NULL, 0, 4*3);

	return EXIT_SUCCESS;
}
