
# Maze2Mesh

## Purpose

Generate (Wavefront .obj) 3D mesh from 2D ASCII tilemap/maze description.

You will almost certainly find no use for this. If I'm wrong, let me know by pushing the star button.

## Tilemap Format

See the example in [data directory](data/bt1skarabrae.txt). It's very ad-hoc.

## Build

Requires [meshoptimizer](https://github.com/zeux/meshoptimizer).

```console
$ MESHOPTDIR=/path/to/meshoptimizer make
```

## Usage

```console
$ ./maze2mesh data/bt1skarabrae.txt
```
