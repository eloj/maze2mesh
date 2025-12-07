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

const float f_min = std::numeric_limits<float>::lowest();
const float f_max = std::numeric_limits<float>::max();

struct Vertex {
	float x,y,z;
};

using VertexArray = std::vector<Vertex>;
using IndexBuffer = std::vector<unsigned int>;
using BBox = Vertex[2];

struct Mesh {
	Mesh() : bbox({ f_max, f_max, f_max }, { f_min, f_min, f_min }) { }
	void optimize(void);

	std::string name;
	VertexArray vertices;
	IndexBuffer indeces;
	BBox bbox;
};

void Mesh::optimize(void) {
	size_t index_count = indeces.size();
	size_t vertex_count = vertices.size();

	printf("Optimizing %s: %zu vertices -> ", name.c_str(), vertex_count);

	IndexBuffer remap(vertex_count);

	size_t opt_vertex_count = meshopt_generateVertexRemap(&remap[0], &indeces[0], index_count, &vertices[0], vertex_count, sizeof(Vertex));

	VertexArray opt_vertices(opt_vertex_count);
	IndexBuffer opt_indeces(index_count);

	meshopt_remapIndexBuffer(&opt_indeces[0], &indeces[0], index_count, &remap[0]);
	meshopt_remapVertexBuffer(&opt_vertices[0], &vertices[0], vertex_count, sizeof(Vertex), &remap[0]);

	vertices = std::move(opt_vertices);
	indeces = std::move(opt_indeces);

	printf("%zu vertices.\n", opt_vertex_count);
}

struct Maze {
	int w;
	int h;
	std::vector<unsigned char> data;

	Mesh	maze;
	Mesh    houses;
	Mesh	floor;
	Mesh	ceiling;
};

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

void write_mesh(FILE *f, const Mesh& mesh, int& total_vertex_count) {
	if (mesh.vertices.size() == 0) {
		return;
	}

	fprintf(f, "o %s\n", mesh.name.c_str());

	for (const Vertex& v : mesh.vertices) {
		fprintf(f, "v %f %f %f\n", v.x, v.y, v.z);
	}

	fprintf(f, "s 0\n");

	for (size_t i = 0 ; i < mesh.indeces.size() ; i += 3) {
		fprintf(f, "f %d %d %d\n", 1 + total_vertex_count + mesh.indeces[i + 0], 1 + total_vertex_count + mesh.indeces[i + 1], 1 + total_vertex_count + mesh.indeces[i + 2]);
	}

	total_vertex_count += mesh.vertices.size();
}

bool write_map_obj(const char *filename, Maze& map) {

	FILE *fout = fopen(filename, "w");
	if (!fout) {
		return false;
	}

	fprintf(fout, "# maze2mesh -- https://github.com/eloj/maze2mesh\n");

	int total_vertex_count = 0;
	write_mesh(fout, map.maze, total_vertex_count);
	write_mesh(fout, map.houses, total_vertex_count);
	write_mesh(fout, map.floor, total_vertex_count);
	write_mesh(fout, map.ceiling, total_vertex_count);
	fclose(fout);

	printf("Final vertex count: %d\n", total_vertex_count);

	return true;
}

void add_bbox_plane(Mesh &mesh, const BBox& bbox, float ypos) {
	const int scale = 1;
	int base_vrt = mesh.vertices.size();

	Vertex rectverts[] = {
		{ bbox[1].x, ypos, bbox[1].z },
		{ bbox[1].x, ypos, bbox[0].z },
		{ bbox[0].x, ypos, bbox[0].z },
		{ bbox[0].x, ypos, bbox[1].z }
	};

	int rectidx[] = { 0, 1, 3, 1, 2, 3 };

	for (Vertex& v : rectverts) {
		v.x *= scale;
		v.y *= scale;
		v.z *= scale;
		mesh.vertices.push_back(v);
	}

	for (int i : rectidx) {
		mesh.indeces.push_back(base_vrt + i);
	}
}

void add_box_at(Maze& map, int x, int y, Mesh& mesh) {
	const int scale = 1;
	int base_vrt = mesh.vertices.size();

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

		if (v.x < mesh.bbox[0].x) { mesh.bbox[0].x = v.x; }
		if (v.y < mesh.bbox[0].y) { mesh.bbox[0].y = v.y; }
		if (v.z < mesh.bbox[0].z) { mesh.bbox[0].z = v.z; }
		if (v.x > mesh.bbox[1].x) { mesh.bbox[1].x = v.x; }
		if (v.y > mesh.bbox[1].y) { mesh.bbox[1].y = v.y; }
		if (v.z > mesh.bbox[1].z) { mesh.bbox[1].z = v.z; }

		mesh.vertices.push_back(v);
	}

	for (int i : boxind) {
		mesh.indeces.push_back(base_vrt + i);
	}
}

int main(int argc, char *argv[]) {
	const char *filename = argc > 1 ? argv[1] : "data/bt1skarabrae.txt";
	bool do_write_tilemap = true;
	bool do_zero_unknown_tiles = false;
	bool do_meshopt = true;
	bool do_floor = true;
	bool do_ceil = false;

	Maze map;
	if (!load_maze(filename, map)) {
		fprintf(stderr, "Error loading map '%s': %s\n", filename, strerror(errno));
		return EXIT_FAILURE;
	}

	printf("Loaded %dx%d map '%s'\n", map.w, map.h, filename);

	map.maze.name = "maze";
	map.houses.name = "houses";
	for (int j = 0 ; j < map.h ; ++j) {
		for (int i = 0 ; i < map.w ; ++i) {
			int idx = j * map.h + i;
			switch (map.data[idx]) {
				case '*':
					add_box_at(map, i, j, map.maze);
					printf("#");
					break;
				case ' ':
					printf(" ");
					break;
				default:
					if ((map.data[idx] >= 'A') && (map.data[idx] <= 'Z')) {
						add_box_at(map, i, j, map.houses);
						printf("%c", map.data[idx]);
					} else if (do_zero_unknown_tiles) {
						map.data[idx] = 0;
						printf("?");
					} else {
						printf("%c", map.data[idx]);
					}
			}
		}
		printf("\n");
	}

	printf("%s", std::format("Maze bounding box = {}\n", map.maze.bbox).c_str());
	// printf(std::format("%f", "House bounding box = {}\n", map.houses.bbox).c_str());

	if (do_floor) {
		printf("Adding floor rectangle.\n");
		map.floor.name = "floor";
		add_bbox_plane(map.floor, map.maze.bbox, map.maze.bbox[0].y);
	}

	if (do_ceil) {
		printf("Adding ceiling rectangle.\n");
		map.ceiling.name = "ceiling";
		add_bbox_plane(map.ceiling, map.maze.bbox, map.maze.bbox[1].y);
	}

	if (do_meshopt) {
		map.maze.optimize();
		map.houses.optimize();
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
	if (!write_map_obj(outfile, map)) {
		fprintf(stderr, "Error writing mesh '%s': %s\n", outfile, strerror(errno));
		return EXIT_FAILURE;
	}
	printf("Wrote mesh to '%s'\n", outfile);

	return EXIT_SUCCESS;
}
