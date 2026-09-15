# Compiler architecture

The compiler uses a sequence of text formats. This keeps each phase independent and makes it possible to inspect what the compiler knows after every step.

Consider this expression inside a program:

```c
return a + b;
```

## 1. Scanning

`wlp4scan` applies the embedded deterministic finite automaton (DFA) to the source characters. It performs maximal munch: it keeps extending a token while a DFA transition exists, then emits the longest accepted token. The example contains tokens such as:

```text
RETURN return
ID a
PLUS +
ID b
SEMI ;
```

The first word is the token kind used by the grammar. The second is the original lexeme. Comments and whitespace do not become tokens.

## 2. Parsing

`wlp4parse` reads those tokens and uses the embedded LR transition and reduction tables. A shift places a token and parser state on their stacks. A reduction replaces the right-hand side of a grammar production with its left-hand nonterminal. The same reductions assemble a preorder parse tree, including a full root:

```text
start BOF procedures EOF
```

For `a + b`, part of the tree records `expr expr PLUS term`. This structure represents precedence from the grammar rather than relying on the original punctuation alone.

## 3. Type checking

`wlp4type` unwraps the `start` root, builds a symbol table for each procedure, verifies declarations and expressions, and prints the original tree with annotations. If `a` and `b` are declared as `long`, representative lines are:

```text
ID a : long
ID b : long
expr expr PLUS term : long
```

Pointer rules are checked here too. For example, dereferencing requires `long*`, while `wain` must return `long` and its second parameter must be `long`. An undeclared name or incompatible operation stops the pipeline.

## 4. Code generation

`wlp4gen` walks the typed tree recursively. It assigns stack-frame offsets to parameters and local variables, emits labels for control flow, and generates ARM64-style CS241 instructions. Expression results use `x0`; helper routines save intermediate values on the stack and load imported runtime addresses before calls.

The output starts with:

```asm
.import print
.import init
.import new
.import delete
b wain
```

These imports explain why generated assembly needs the matching CS241 assembler, linker, runtime objects, and emulator to execute.

## Why files connect the phases

The driver writes every phase to a temporary file before running the next phase. This makes failure handling precise: a nonzero status stops immediately, diagnostics remain visible on standard error, and incomplete output never replaces the requested `.asm` file. The temporary files are removed when compilation finishes.
