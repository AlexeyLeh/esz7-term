# xme0724 ADC Linux Console
## Terminal for Xilinx SoC xme0724-20

## Purpose
A terminal application for testing the basic functionality of the xme0724-20 SoC. \

The board also features an ADS125 ADC, which can be controlled via this application.

The source code is designed to run either as a bare-metal application or as an ARM-based application. \
In the former case, the code is used as an "Application" within the Xilinx Vivado/Vitis environment; \
in the latter case, it is built using cross-compilation to generate an ARM-compatible application.

## Bare-metal build (PS variant)
- Launch Xilinx SDK (Vitis). \
- Create a new Application Project (New Application Project) and select "Empty Project." \
- Import the source files: Import -> File System -> Browse -> Select files -> Finish. \
- Compilation should trigger automatically if "Build Automatically" is enabled; \
otherwise, you can initiate the build manually via the menu. \
- Run the application using "Run As - 1" (Ctrl+F11) or "Debug As - 1" (F11).

Before running the application, you must load the FPGA firmware (a *.bit file). \
(Refer to the Xilinx menu: "Program FPGA".) \

## ARM application build
- You must have CMake and `make.exe` installed. \
- You must also have a toolchain installed for building ARM applications. \
For example: [Arm GNU Toolchain Downloads](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)

Any suitable compiler can be used, such as **arm-none-gnueabihf-gcc**. You can perform the build either from the command line or from any development environment you are comfortable with (for example, JetBrains CLion or FAR).
The rest is up to your personal preference...

# CMake Build
The build process is defined in the `CMakeLists.txt` file.

Pay special attention to the following CMake variables:
- `set(CMAKE_SYSTEM_NAME Generic)`
- `set(CMAKE_C_COMPILER "arm-none-linux-gnueabihf-gcc.exe")`

These variables can be used directly within `CMakeLists.txt` or configured in your development environment's settings.

Preparing the build (ensure the paths to the utilities are set in your OS environment, or specify them explicitly in the command):
```
$ cd src
$ cmake -G "MinGW Makefiles" -DCMAKE_SYSTEM_NAME=Generic -DCMAKE_C_COMPILER=arm-none-linux-gnueabihf-gcc.exe -B debug
```
or
```
$ mkdir build
$ cd build
$ cmake -G "MinGW Makefiles" -DCMAKE_SYSTEM_NAME=Generic -DCMAKE_C_COMPILER=arm-none-linux-gnueabihf-gcc.exe
```

Build: `$ make -C build`

Clean: `$ make -C build clear`

Execution: Copy the **adc-term** binary to the xme0724 device and run it using `$ ./adc-term`

# Make Build
[*The Makefile will be published soon.*]

In short:
Create a basic `Makefile` that includes your `*.c` and `*.h` source files,
and add the following variables:
```
- CC=<path to>arm-none-linux-gnueabihf.exe
- CFLAGS=-static
```
Run: `$ make`

