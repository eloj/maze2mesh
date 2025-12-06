/*
	maze2mesh -- Generate mesh from 2D cartesian ASCII description.
	Copyright (c) 2025, Eddy Jansson. Licensed under The MIT License.

	See https://github.com/eloj/maze2mesh
*/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#include <vector>
#include <limits>
#include <format>

#include "meshoptimizer.h"

struct Vertex {
	float x,y,z;
};

struct Maze {
	int w;
	int h;
	std::vector<unsigned char> data;
};

using VertexArray = std::vector<Vertex>;
using IndexBuffer = std::vector<unsigned int>;
using BBox = Vertex[2];

template<>
struct std::formatter<Vertex> {
	constexpr auto parse(std::format_parse_context& ctx) {
		return ctx.begin();
	}

	auto format(const Vertex& v, std::format_context& ctx) const {
		return std::format_to(ctx.out(), "{},{},{}", v.x, v.y, v.z);
	}
};

template<>
struct std::formatter<BBox> {
	constexpr auto parse(std::format_parse_context& ctx) {
		return ctx.begin();
	}

	auto format(const BBox& bbox, std::format_context& ctx) const {
		return std::format_to(ctx.out(), "{{ {{ {} }}, {{ {} }} }}", bbox[0], bbox[1]);
	}
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

		int len = (int)strlen(line) - 1;
		if (len > max_w) {
			max_w = len;
		}
		++max_h;
	}

	rewind(f);

	map.w = max_w;
	map.h = max_h;
	map.data.assign(max_w * max_h, 0);

	int idx = 0;
	while ((nread = getline(&line, &line_size, f)) != -1) {
		if (!line || !*line || *line == ';') {
			continue;
		}

		size_t len = strlen(line) - 1;

		assert((int)len <= max_w);
		assert((int)len + idx <= (int)map.data.size());

		memcpy(&map.data[idx], line, len);

		idx += map.w;
	}
	free(line);
	fclose(f);

	return true;
}

bool write_obj(const char *filename, VertexArray& vertices, IndexBuffer& indeces) {

	FILE *fout = fopen(filename, "w");
	if (!fout) {
		return false;
	}

	fprintf(fout, "# maze2mesh\n");
	fprintf(fout, "o maze\n");

	for (Vertex& v : vertices) {
		fprintf(fout, "v %.06f %.06f %06f\n", v.x, v.y, v.z);
	}

	fprintf(fout, "s 0\n");

	for (size_t i = 0 ; i < indeces.size() ; i += 3) {
		fprintf(fout, "f %d %d %d\n", 1 + indeces[i + 0], 1 + indeces[i + 1], 1 + indeces[i + 2]);
	}

	fclose(fout);
	return true;
}

void add_box_at(Maze& map, int x, int y, VertexArray& vertices, IndexBuffer& indeces, BBox &bbox) {
	int scale = 2;
	int base_vrt = vertices.size();

	Vertex boxverts[] = {
		{ 1.0, 1.0, -1.0 },
		{ 1.0, 0.0, -1.0 },
		{ 1.0, 1.0,  0.0 },
		{ 1.0, 0.0,  0.0 },
		{ 0.0, 1.0, -1.0 },
		{ 0.0, 0.0, -1.0 },
		{ 0.0, 1.0,  0.0 },
		{ 0.0, 0.0,  0.0 }
	};

	int boxind[] = {
		4, 2, 0, 2, 7, 3,
		6, 5, 7, 1, 7, 5,
		0, 3, 1, 4, 1, 5,
		4, 6, 2, 2, 6, 7,
		6, 4, 5, 1, 3, 7,
		0, 2, 3, 4, 0, 1
	};

	for (Vertex& v : boxverts) {
		v.x *= scale;
		v.y *= scale;
		v.z *= scale;

		v.x += (x - map.w/2) * scale;
		v.z += (y - map.h/2) * scale;

		if (v.x < bbox[0].x) { bbox[0].x = v.x; }
		if (v.y < bbox[0].y) { bbox[0].y = v.y; }
		if (v.z < bbox[0].z) { bbox[0].z = v.z; }
		if (v.x > bbox[1].x) { bbox[1].x = v.x; }
		if (v.y > bbox[1].y) { bbox[1].y = v.y; }
		if (v.z > bbox[1].z) { bbox[1].z = v.z; }

		vertices.push_back(v);
	}

	for (int i : boxind) {
		i += base_vrt;
		indeces.push_back(i);
	}
}

int main(int argc, char *argv[]) {
	const char *filename = argc > 1 ? argv[1] : "data/bt1skarabrae.txt";
	bool do_write_tilemap = true;
	bool do_meshopt = true;

	Maze map;
	if (!load_maze(filename, map)) {
		fprintf(stderr, "Error loading map '%s': %s\n", filename, strerror(errno));
		return EXIT_FAILURE;
	}

	printf("Loaded %dx%d map '%s'\n", map.w, map.h, filename);

	const float f_min = std::numeric_limits<float>::lowest();
	const float f_max = std::numeric_limits<float>::max();

	VertexArray vertices;
	IndexBuffer indeces;
	BBox bbox{ { f_max, f_max, f_max }, { f_min, f_min, f_min } };

	for (int j = 0 ; j < map.h ; ++j) {
		for (int i = 0 ; i < map.w ; ++i) {
			int idx = j * map.h + i;
			if (map.data[idx] == '*') {
				add_box_at(map, i, j, vertices, indeces, bbox);
				printf("#");
			} else {
				printf(" ");
			}

		}
		printf("\n");
	}

	printf(std::format("Bounding box = {}\n", bbox).c_str());

	size_t index_count = indeces.size();
	size_t unindexed_vertex_count = vertices.size();
	printf("Generated %zu vertices, %zu indeces\n", unindexed_vertex_count, index_count);

	if (do_meshopt) {
		IndexBuffer remap(unindexed_vertex_count);

		size_t vertex_count = meshopt_generateVertexRemap(&remap[0], &indeces[0], index_count, &vertices[0], unindexed_vertex_count, sizeof(Vertex));
		printf("Optimized vertex count: %zu\n", vertex_count);

		VertexArray opt_vertices(vertex_count);
		IndexBuffer opt_indeces(index_count);

		meshopt_remapIndexBuffer(&opt_indeces[0], &indeces[0], index_count, &remap[0]);
		meshopt_remapVertexBuffer(&opt_vertices[0], &vertices[0], unindexed_vertex_count, sizeof(Vertex), &remap[0]);

		vertices = opt_vertices;
		indeces = opt_indeces;
	}

	if (do_write_tilemap) {
		const char *outtilemap = "maze1.tilemap.bin";
		FILE *f = fopen(outtilemap, "wb");
		if (f) {
			fwrite(&map.w, sizeof(map.w), 1, f);
			fwrite(&map.h, sizeof(map.h), 1, f);
			fwrite(&map.data[0], map.data.size(), 1, f);
			fclose(f);
			printf("Wrote tilemap data to '%s'\n", outtilemap);
		} else {
			fprintf(stderr, "Error writing tilemap '%s': %s\n", outtilemap, strerror(errno));
			return EXIT_FAILURE;
		}
	}

	const char *outfile = "maze1.obj";
	if (!write_obj(outfile, vertices, indeces)) {
		fprintf(stderr, "Error writing mesh '%s': %s\n", outfile, strerror(errno));
		return EXIT_FAILURE;
	}
	printf("Wrote mesh to '%s'\n", outfile);

	return EXIT_SUCCESS;
}
