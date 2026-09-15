# WLP4 compiler for ARM64

This repository packages a compiler developed while studying compiler construction in CS241. It translates the course's `long`-based WLP4 language into the ARM64-style assembly understood by the CS241 toolchain. The implementation is split into four small command-line programs so each compiler phase can also be studied on its own.

## Compiler pipeline

```text
program.wlp4
    -> wlp4scan  (tokens)
    -> wlp4parse (parse tree)
    -> wlp4type  (typed parse tree)
    -> wlp4gen   (ARM64-style assembly)
```

Each program reads standard input, writes its successful result to standard output, and writes errors to standard error. `scripts/compile.sh` connects the stages with checked intermediate files. If any stage fails, later stages do not run and the requested assembly output is removed.

## Supported language features

The included examples and regression tests exercise:

- `long` and `long*` declarations
- arithmetic, comparisons, assignment, `if`/`else`, and `while`
- procedures, parameters, calls, and `wain`
- `println`, pointers, `new long[...]`, and `delete []`

The compiler also contains grammar support for `putchar` and `getchar`. Runtime behavior for generated programs has not been verified in this repository because the CS241 assembler, linker, emulator, and runtime objects are not installed in the test environment.

## Prerequisites

- GNU Make
- a C++17 compiler (`g++`, `clang++`, or `c++`)
- Bash, for the compiler driver and regression tests

No course tools are needed to build the compiler or generate assembly.

## Build and use

```sh
make -j4
make compile INPUT=examples/add.wlp4
make compile INPUT=examples/add.wlp4 OUT=build/my-program.asm
make examples -j4
make test
make help
make clean
```

The default output for `make compile INPUT=examples/add.wlp4` is `build/add.asm`. Override the compiler or flags when needed:

```sh
make CXX=clang++ CXXFLAGS='-std=c++17 -Wall -Wextra -pedantic -O0 -g'
```

A minimal input program looks like this:

```c
long wain(long a, long b) {
  long sum = 0;
  sum = a + b;
  println(sum);
  return sum;
}
```

## Repository layout

```text
src/                  scanner, parser, type checker, and code generator
examples/             valid WLP4 programs covering major features
tests/                 regression suite and invalid input cases
scripts/compile.sh     checked source-to-assembly pipeline
docs/architecture.md   walkthrough of the compiler phases
bin/                   generated compiler executables
build/                 generated trees, test files, and assembly
```

`bin/` and `build/` are generated and ignored by Git.

## Assembly and course runtime

Generated assembly uses CS241's ARM64 syntax and begins with imports for `print`, `init`, `new`, and `delete`. It is not intended for the host system assembler. Existing course exercises use commands named `cs241.linkasm`, `cs241.linker`, and `cs241.arm64emu`, plus course-supplied `.com` runtime objects. Those tools and runtime objects are course infrastructure and are not bundled here.

Once you have the matching course environment, the broad workflow is to assemble the generated `.asm`, link it with the required runtime objects, and execute the linked image with `cs241.arm64emu`. Exact runtime object names and argument conventions depend on the provided course setup.

## Known limitations

- Tests validate scanning, tree construction, semantic rejection, stage integration, and assembly structure. They do not prove the runtime behavior of generated assembly.
- The embedded DFA and LR parsing tables in `wlp4scan.cc` and `wlp4data.h` are supplied course data used by the original assignment implementation.
- This repository has no declared license. Public visibility alone does not grant reuse rights.

## Attribution

The compiler source began as a student CS241 assignment implementation. The scanner automaton and parser grammar/tables are embedded course-supplied data. The repository packaging, portability fixes, tests, and documentation organize that work as a standalone educational project.
