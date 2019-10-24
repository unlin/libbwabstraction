# libbwabstraction
Code for the paper "Scale-aware Black-and-White Abstraction of 3D Shapes" in SIGGRAPH 2018. Please visit [our project webpage](https://cgv.cs.nthu.edu.tw/projects/Shape_Analysis/BW_Abstraction) to learn more about this paper.

## Features

#### C++ Library
The algorithm described in our paper is wrapped inside a simple, easy-to-use C++ interface. Check this snippet out:

```C++
  // initialize data structures for libbwabstraction
  bwabstraction::Parameters param;
  bwabstraction::BWAbstraction bwa;
  bwabstraction::Result result;

  // load camera mvp and model file
  param.LoadMVPMatrixFromFile("camera.txt");
  bwa.LoadModel("model.obj", param);
    
  // render with desired parameters
  param.renderWidth = 3200;
  param.rednerHeight = 2400; // just to show a few options...
  bwa.Render(&result, param);
  
  // the results are available as cv::Mat images
  cv::imwrite("bwaImage.png", result.bwaImage);
```

#### Example Code & CLI Tool
We have documented example codes and a CLI tool for easy use. The tool works like:

`> bwa_cli model.obj camera.txt bwaImage.png --scale=0.8`

#### GUI Tool
Further more, a Qt-based GUI tool is provided for quick visualization and interactive control of the algorithm.

![image](https://github.com/unlin/libbwabstraction/blob/master/qtgui_01.png?raw=true)
![image](https://github.com/unlin/libbwabstraction/blob/master/qtgui_02.png?raw=true)

## Prebuilt Binary
We provide Windows prebuilt binaries, including example code executable, command line tool, GUI tools and development library. Take a look at the [latest release](https://github.com/unlin/libbwabstraction/releases/latest)!

## Compile
If the prebuilt binaries are not good enough for you, we also provide configured build script for you. Everything can be built on both Windows x64 and MacOSX.

#### Dependencies
Before you can compile the library, the following dependency libraries should be installed on your system.
* [Boost C++ Libaries](https://www.boost.org/)
* [CGAL](https://www.cgal.org/)
* [Eigen](http://eigen.tuxfamily.org/)
* [GLEW](http://glew.sourceforge.net/)
* [GLFW](https://www.glfw.org/)
* [glm](https://glm.g-truc.net/)
* [OpenCV](https://opencv.org/)
* [opengm](https://github.com/opengm/opengm)
* [OpenMesh](https://www.openmesh.org/)
* [libgmp](https://gmplib.org/)
* [libmpfr](https://www.mpfr.org/)
* [cxxopts](https://github.com/jarro2783/cxxopts)

#### Windows x64
Use the Visual Studio 2015 solution `/vc14/libbwabstraction.sln`. Two configurations are available: x64 Debug and x64 Release. For the Qt GUI application, use the Qt project `/tools/qtgui/qtgui.pro`.

#### MacOSX
Use [CMake](https://cmake.org/) to build the provided `/cmakelist.txt`. For the Qt GUI application, use the Qt project `/tools/qtgui/qtgui.pro`.
