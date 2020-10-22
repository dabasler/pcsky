# pcsky
 Transform UAV point clouds to hemispherical images to extract  Sky View Factor, LAI and other canopy parameters.
 
![pcsky](doc/pcsky.png)

Point cloud equivalent to [CanpoyGapAnalyzer](https://github.com/dabasler/CanopyGapAnalyzer), which was designed for real world images.
 
The tool currently reads large points pointclouds provided in PLY format. Colors are supported.

### Build

Dependencies:
* [miniply](https://github.com/vilya/miniply) (place miniply.h and miniply.cpp in the /src folder)
* libtiff

to build run `make` (to build tests as well, run `make tests` afterwards)
### Run

Provide the following arguments to run the program
```sh
./bin/pcsky [pointcloud.ply] c=[X,Y,Z] r=[radius] [outputfilepattern]
```

The arguments are
* `[pointcloud.ply]` name of the input pointcloud
* `c=[X,Y,Z]` camera position (only points above camera Z position will be considered)
* `r=[radius]` radius of the hemispherical image [in pixels]
* `[outputfilepattern]` path and basename for the output files

Example
```sh
./bin/pcsky foo.ply c=1.0,2.0,-3.0 r=256 ./output/bar
```

### Note
 This project is under heavy development and may not yet produce stable results.
