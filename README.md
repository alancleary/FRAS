# FRAS: Folklore Random Access for SLPs

This repo contains a modified version of the folklore algorithm for random access to grammar-compressed strings.
The original folklore algorithm only works on grammars in Chomsky normal form and is not space efficient.
This implementation generalizes the folklore algorithm to work on any straight-line program (SLP) grammar.
This includes RePair style grammars (which are often called SLP grammars), which are grammars in Chomsky normal form except for the start rule.

Currently this implementation can load grammars generated using RePair (Navarro's implementation), BigRePair, and MR-RePair.
Loaded grammars are represented as an array-of-arrays.
This implementation also supports compression of these arrays using bit packing.
Three different bit packing strategies are implemented, each with its own space-time trade-offs.

## Building

The project uses the [CMake](https://cmake.org/) meta-build system to generate build files specific to your environment.
Generate the build files as follows:
```console
cmake -B build .
```
This will create a `build/` directory containing all the build files.

To build the code in using the files in the `build/` directory, run:
```console
cmake --build build
```
This will generate an `fras` executable in the `build/` directory.
If you make changes to the code, you only have to run this command to recompile the code.


## Running

`fras` uses a command-line interface (CLI).
Its usage instructions are as follows:
```console
usage: ./build/fras <type> <filename> <encoding> <querysize> [numqueries=10000] [seed=random_device]

args:
	type={mrrepair|navarro|bigrepair}: the type of grammar to load
		mrrepair: for grammars created with the MR-RePair algorithm
		navarro: for grammars created with Navarro's implementation of RePair
		bigrepair: for grammars created with Manzini's implementation of Big-Repair
    ecoding={array|bpleft|bpright|bpmono}: how the grammar should be encoded in memory
		array: an array of arrays (fastest)
		bpleft: bit packing where the pack width of a rule is detrmined by its non-terminal character
		bpright: bit packing where the pack width of a rule is the smallest that is compatible with every character in the rule
		bpmono: same as bpright but a rule's pack width cannot be less than the previous rule's width
	filename: the name of the grammar file(s) without the extension(s)
	querysize: the size of the substring to query for when benchmarking
	numqueries: the number of queries to run when benchmarking
	seed: the seed to use with the pseudo-random number generator
```

What the program outputs depends on what is currently being developed.
Generally, information for the user will be sent to the standard error and program outputs, such as strings generated from random access queries, will be sent to the standard output.
For this reason, it's recommended to always redirect the standard output to a file.
```console
./build/fras <type> <filename> <encoding> <querysize> &> /dev/null
```
