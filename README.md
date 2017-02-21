Space-efficient computation of the Re-Pair grammar
===============
Author: Nicola Prezza (nicola.prezza@gmail.com)

From the paper: Bille P, Gørtz IL, Prezza N. Space-Efficient Re-Pair Compression. arXiv preprint arXiv:1611.01479. 2016 Nov 4. (accepted at DCC 2017)

### Description

This code computes the Re-Pair grammar of a input ASCII file using roughly (2n + eps) * log n bits of RAM during execution, where eps>0 is a small constant. Running time is linear. 

### Download

To clone the repository, call

> git clone http://github.com/nicolaprezza/Re-Pair

### Compile

The library has been tested under linux using gcc 5.4.0. 

You can use use cmake to generate the Makefile. Create a build folder in the main Re-Pair folder:

> mkdir build

execute cmake:

> cd build; cmake ..

and compile:

> make

### Run

After compiling, run 

>  repair-compress input.txt

This command produces the compressed file input.txt.rp. To decompress, run

>  repair-decompress input.txt.rp

This command produces the decompressed file input.txt.rp.decompressed

