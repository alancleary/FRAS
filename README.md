# SLP Folklore Algorithm

This repo contains a modified version of the folklore algorithm for random access to grammar-compressed strings.
The original folklore algorithm only works on grammars in Chomsky normal form and is not space efficient.
This implementation generalizes the folklore algorithm to work on any straight-line program (SLP) grammar.
This includes RePair style grammars (which are often called SLP grammars), which are grammars in Chomsky normal form except for the start rule.

Currently this implementation can load grammars generated using RePair (Navarro's implementation), BigRePair, and MR-RePair.

## Building

The project uses the [CMake](https://cmake.org/) meta-build system to generate build files specific to your environment.
Generate the build files as follows:
```bash
cmake -B build .
```
This will create a `build/` directory containing all the build files.

To build the code in using the files in the `build/` directory, run:
```bash
cmake --build build
```
This will generate an `SLP-folklore` executable in the `build/` directory.
If you make changes to the code, you only have to run this command to recompile the code.


## Running

`SLP-folklore` uses a command-line interface (CLI).
Its usage instructions are as follows:
```bash
usage: ./build/SLP-folklore <type> <filename>

args:
	type={mrrepair|navarro|bigrepair}: the type of grammar to load
		mrrepair: for grammars created with the MR-RePair algorithm
		navarro: for grammars created with Navarro's implementation of RePair
		bigrepair: for grammars created with Manzini's implementation of Big-Repair
	filename: the name of the grammar file(s) without the extension(s)
```

What the program outputs depends on what is currently being developed.
Generally, information for the user will be sent to the standard error and program outputs, such as strings generated from random access queries, will be sent to the standard output.
For this reason, it's recommended to always redirect the standard output to a file.
```bash
./build/SLP-folklore <type> <filename> &> /dev/null
```
