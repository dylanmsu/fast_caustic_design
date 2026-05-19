# Fast caustic design: A fast high contrast freeform optics designer based on OTMap

## Contact

This repository is an early open source prototype with significant limitations. A fully reworked commercial version supporting generic shapes and nearby point light sources is in active development. For custom pieces or professional enquiries, visit [dyopton.be](https://dyopton.be/) or contact dylan@dyopton.be.

## Examples

#### Ring:
![ring simulation](data/ring_sim.png)

```
./caustic_design -res 512 -focal_l 1.5 -thickness 0.2 -width 1 -in_trg ../data/ring.png
```

#### Einstein:
![einstein simulation](data/einstein_sim.png)

```
./caustic_design -res 512 -focal_l 1.5 -thickness 0.3 -width 1 -in_trg ../data/einstein.png
```

## Usage

Download the latest build from [Releases](https://github.com/dylanmsu/fast_caustic_design/releases). 

Run the command of one of the examples with the image locations properly filled out. After its complete the 3d model will be located in the directory above the directory of the exe.
## Build from source

This code uses [Eigen](https://eigen.tuxfamily.org), Surface_mesh, and CImg that are already included in the repo/archive.
The only libraries you need to install are [Ceres Solver](http://ceres-solver.org/) for the normal integration and libpng/libjpg for image IO.

It is however highly recommended to install [SuiteSparse/Cholmod](http://faculty.cse.tamu.edu/davis/suitesparse.html) for higher performance.

### Install dependencies on Windows:
Install vcpkg:
```bash
$ cd C:\
$ git clone https://github.com/microsoft/vcpkg.git
$ cd vcpkg
$ .\bootstrap-vcpkg.bat
```

Install Ceres solver and its dependencies:
```bash
$ ./vcpkg.exe install ceres[core] --triplet x64-windows-static
```

Install libpng
 and its dependencies:
```bash
$ vcpkg install libpng --triplet x64-windows-static
```

All you then need to do is to clone the repo, configure a build directory with cmake, and then build.

Make sure the path in CMAKE_TOOLCHAIN_FILE is correct.

````bash
$ git clone --recursive git@github.com:dylanmsu/fast_caustic_design.git
$ cd fast_caustic_design
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded -DCMAKE_BUILD_TYPE=Release
$ cmake --build . --config Release
$ ./Release/caustic_design.exe -res 512 -focal_l 1.5 -thickness 0.3 -width 1 -in_trg ../data/einstein.png
````

## How does it work
Creating a lens surface whose shadow matches a target image involves two main steps:

### 1. Figuring out how to move the light
First, we need to determine how light should travel from the lens surface to the target shadow. This is done using a technique called optimal transport (OT). OT is a challenging problem and an area of ongoing research. It gives us a map that tells each point on the lens surface where its light should land on the target image.

Traditionally, this problem is solved using structures called power diagrams (also known as Laguerre-Voronoi diagrams). However, in our case, we use a newer, faster method.

The usage of optimal transport is not just to reduce surface variations. It is actually quite important to have a truly optimal transport plan. According to Brenier's theorem, an optimal transport plan is the gradient of a potential. Without this fact we cannot approximate the inverse gradient (or normal integration) afterwards accurately.

### 2. Shaping the surface to steer the light
Once we have the transport map, the next step is to figure out the exact geometry of the lens surface. For each vertex on the surface, we use the OT map to set the x and y direction of the outgoing ray. The z-direction is chosen based on the focal length.

These rays define how we want the surface to bend the light. Using inverse Snell’s law, we compute the target surface normals needed to steer the rays correctly. Finally, we use a solver (Ceres) to adjust the vertex positions on the lens surface so that the computed normals match the target ones. This is called normal integration and can be thought of as approximating the inverse gradient of the vertex normals.

## License

The core of the transport solver and caustic design utility is provided under the [GNU Public License v3](https://www.gnu.org/licenses/gpl-3.0.html).

## References

[1] Georges Nader and Gael Guennebaud. _Instant Transport Maps on 2D Grids_. ACM Transactions on Graphics (Proceedings of Siggraph Asia 2018). [[pdf]](https://hal.inria.fr/hal-01884157) [[video]](https://www.youtube.com/watch?v=Ofz4-reJQRk)
