#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>

#include <vector>
#include <array>

#include "meshoptimizer.h"

struct Maze {
	int w;
	int h;
	std::vector<unsigned char> data;
};

bool load_maze(const char *filename, Maze& map) {

	FILE *f = fopen(filename, "rb");
	if (!f) {
		return false;
	}

	char *line = NULL;
	size_t line_size = 0;
	ssize_t nread;

	int max_w = 0;
	int max_h = 0;

	// First, just figure out the dimensions.
	while ((nread = getline(&line, &line_size, f)) != -1) {
		if (!line || !*line || *line == ';') {
			continue;
		}

		size_t len = strlen(line);

		if (line[len - 1] == '\n') {
			line[len - 1] = '\0';
			--len;
		}

		++max_h;
		if ((int)len > max_w) {
			max_w = len;
		}

		// printf("XXX:%s\n", line);
	}
	// printf("Detected map dimensions: %dx%d\n", max_w, max_h);

	rewind(f);

	map.w = max_w;
	map.h = max_h;
	map.data.assign(max_w * max_h, 0);

	int idx = 0;
	while ((nread = getline(&line, &line_size, f)) != -1) {
		if (!line || !*line || *line == ';') {
			continue;
		}

		size_t len = strlen(line);

		if (line[len - 1] == '\n') {
			line[len - 1] = '\0';
			--len;
		}
		assert((int)len <= max_w);
		assert((int)len + idx <= (int)map.data.size());

		memcpy(&map.data[idx], line, len);

		idx += map.w;
	}
	free(line);
	fclose(f);

	return true;
}

int main(int argc, char *argv[]) {
	const char *filename = argc > 1 ? argv[1] : "data/bt1skarabrae.txt";

	Maze map;
	if (!load_maze(filename, map)) {
		fprintf(stderr, "Error loading map '%s': %s\n", filename, strerror(errno));
		return EXIT_FAILURE;
	}

	printf("Loaded %dx%d map '%s'\n", map.w, map.h, filename);

	for (int j = 0 ; j < map.h ; ++j) {
		for (int i = 0 ; i < map.w ; ++i) {
			int idx = j * map.h + i;
			printf("%c", map.data[idx]);

		}
		printf("\n");
	}

	meshopt_generateVertexRemap(NULL, NULL, 0, NULL, 0, 4*3);

	return EXIT_SUCCESS;
}
