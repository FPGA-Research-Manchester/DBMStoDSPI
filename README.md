# OrkhestraFPGAStream - Accelerated database operations on an FPGA with a dynamic stream processing interface

[![License](https://img.shields.io/github/license/FPGA-Research-Manchester/OrkhestraFPGAStream)](https://opensource.org/licenses/Apache-2.0)
[![Build & Test](https://github.com/FPGA-Research-Manchester/OrkhestraFPGAStream/actions/workflows/cmake.yml/badge.svg)](https://github.com/FPGA-Research-Manchester/OrkhestraFPGAStream/actions)
[![Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)

OrkhestraFPGAStream is a middleware for orchestrating a dynamic stream processing pipeline. Currently, it only supports DBMS data streams with *DBMStoDSPI*, which uses FPGA modules with our Dynamic Stream Processing Interface (*DSPI*). This software stack is meant to be coupled with a SQL parser or a DBMS. Then the software stack would execute the given query plan with an available FPGA using the available accelerator module library. How the data flows through the FPGA stack is shown in the graph below:

![DBMStoDSPI source layout](./docs/DBMStoDSPI_graph.svg)

As the image shows, the input would be a query plan graph from a DBMS to the software stack, which will use [FOS](https://github.com/FPGA-Research-Manchester/fos) and the modules described below to accelerate the query. The image is slightly out of date, but it gives a rough overview.

## Currently supported data types:

- 32 bit integer
- Fixed length string
- SQL Date
- 64 bit SQL Decimal(15,2)
- Raw binary data
- NULL

## Currently supported modules:

- [Filter](https://docs.google.com/document/d/1aYy9Etr1Ixwe3E7jI4mP0RaP6_JUYoRPTvVph6-HNGM/view)
- [Join](https://docs.google.com/document/d/1r0RVhj606VpfFN-_qD-pGoYDA3DNoBDvr2w0ZUhhkLU/view)
- [Linear Sort](https://docs.google.com/document/d/1rDDLILdMLcuyK8YAFJgvJH6Eq8vFAtm5XBoBC7NX44E/view)
- [Merge Sort](https://docs.google.com/document/d/1PdNX-QX6q9c99VxUFVUgqroxa9sadZm0mU3asQ_sdqQ/view)
- [Addition](https://docs.google.com/document/d/1z2pN-B5mMqBWMHZfsHWlNOEh4y0oQSoJJaQwMiRgKD8/view)
- [Multiplication](https://docs.google.com/document/d/13FvDpvQOcqsJmrKadfZ0wl7sOHxVujxV2PWCQ0EDBH0/view)
- [Global Aggregation Sum](https://docs.google.com/document/d/17INhz4SAK0X97FyJYAxEcWI2qQLcixy8q_eU_KDbDEw/view)
- [Group By](https://docs.google.com/document/d/1Wm6aJAiId8_GNVFq-gOoQ_f0uoqSBnO8gcNFH6yIbvI/view)
- [Regex](https://docs.google.com/document/d/1LQgL-ro1NuhgcH4bcM1f07N-RzCHXtRX4h3esncZgHM/view)
- [Sobel](https://docs.google.com/document/d/1u52Lgn-IQf6aunBRBpJoGi7Sql6Azs4kKVbnnr6M0TE/view)
- [Monochrome converter](https://docs.google.com/document/d/1Q4I0nCbbSqsFrv18iH-wz8p9lwfChO2P7XYtVB4AZ7o/view)
- [*DMA*](https://docs.google.com/document/d/1cxJLcjkrTCrByOmtiYwsu4Ptbp78npZbajhQ878ixp0/view)

The **DMA** is a compulsory module which will set up the interface.
The **sort** modules are meant to be used together.
The **join** module is for sort-merge join operations. So it will only join sorted streams.
The **addition**, **multiplication** and **sum** modules work with 64 bit decimal values.

The interface specification is better described in the [thesis](https://www.research.manchester.ac.uk/portal/files/213182998/FULL_TEXT.PDF) by Kristiyan Manev.

## How does it work?

To run the program use the following command:

```
dbmstodspi -i INPUT_DEF.json -c CONFIG.ini
```

The first file defines the query plan. The second file contains json filepaths to define which bitstreams are available with their respective constraints. Examples can be found here:

* [INPUT_DEF.json](./resources/input_defs/TPCH_Q19_SF01.json)
* [CONFIG.ini](./resources/config.ini)

For the interactive mode for demos or debugging use the -x flag without the -i flag and the INPUT_DEF file.

More info on the how the data is mapped to the interface and how the data is processed can be found [here](./docs/README.md).

## How to use the source?

The documentation for this project is created using [CMake](https://cmake.org/cmake/help/latest/guide/tutorial/index.html) with [Doxygen](https://www.doxygen.nl/manual/starting.html). To turn the documentation build option on you need to change the main [CMakeLists.txt](./CMakeLists.txt) file and then you can find the output in the build directory under the *doc_doxygen* folder. The FOS library and test library aren't included in the generated documentation.

Formatting is done according to the [clang-tidy](https://clang.llvm.org/extra/clang-tidy/) options you can find [here](./.clang-tidy).

Testing for this code is done using [googletest](https://github.com/google/googletest) and is automatically downloaded when the code is built with testing options turned on.

## Quick steps to running the benchmark from zero:

* Get a ZCU102 SD card image [here](https://github.com/FPGA-Research-Manchester/fos/blob/fdac37e188e217293d296d9973c22500c8a4367c/sd_card_images/README.md) or build it [yourself](https://github.com/FPGA-Research-Manchester/fos/blob/fdac37e188e217293d296d9973c22500c8a4367c/sd_card_images/build/README.md)
* Install the OS to an 16GB SD card. Further explanation [here](https://raspberrypi.stackexchange.com/questions/931/how-do-i-install-an-os-image-onto-an-sd-card) or just use the following command:

```
dd if=<fos_image.img> of=<SD card image> bs=4M status=progress
```

* Boot the board from the SD card
* Login with default username *xilinx* password *xilinx* (change the password)
* Allocate UDMA buffer space and possibly extend CMA space as described in the tutorial [here](./docs/memory_allocation.md)
* Build OrkhestraFPGAStream using CMake
* Create the dataset 
  * Get a [TPC-H data generator](https://github.com/databricks/tpch-dbgen) 
  * Use [data generation script](./resources/benchmark/generate_data.sh)
* Run [benchmark perl script](./resources/benchmark/benchmark.pl)