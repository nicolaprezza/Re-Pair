Space-efficient computation of the Re-Pair grammar
===============
Author: Nicola Prezza (nicolapr@gmail.com)

From the paper: Bille P, Gørtz IL, Prezza N. Space-Efficient Re-Pair Compression. arXiv preprint arXiv:1611.01479. 2016 Nov 4.

### Description

This code computes the Re-Pair grammar of a input ASCII file using roughly (2n + sqrt(n)) * log n bits of RAM during execution. Running time is linear. 

### Download

To clone the repository, call

> git clone http://github.com/nicolaprezza/Re-Pair

### Compile

The library has been tested under linux using gcc 5.4.0. You need the SDSL library installed on your system (https://github.com/simongog/sdsl-lite). It is expected that SDSL headers are stored in ~/include/, and SDSL libraries are stored in ~/lib/

You can use use cmake to generate the Makefile. Create a build folder in the main Re-Pair folder:

> mkdir build

execute cmake:

> cd build; cmake ..

and compile:

> make

### Run

After compiling, run 

>  repair input.txt out

This command produces two files: 

1. out.rp: is the compressed text composed of dictionary/alphabet symbols (integers) such that no pair of adjacent symbols appears more than once.
2. output.g contains the Re-Pair grammar. This is a binary file composed of a list of triples (integers written consecutively) XAB representing rule X -> AB. By default, the program uses 32-bit integers. This default behavior can be changed by using a different integer type in the template class repair.hpp.
