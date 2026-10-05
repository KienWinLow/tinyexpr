# Assignment-1
C expression evaluator built on tinyexpr
# uqexpr: Interactive Expression Evaluator (C)

A command-line calculator that reads math expressions, supports named variables and loop variables, and prints results to a configurable number of significant figures. It uses the open-source [tinyexpr](https://github.com/codeplea/tinyexpr) library to parse and evaluate expressions.

Built for **CSSE2310 (Computer Systems Principles and Programming)**, University of Queensland, Semester 1 2025.

## Features

- Evaluates expressions typed interactively or read from an input file.
- **Variables**: assign with `x = 3 * 2` and reuse them in later expressions.
- **Pre-defined variables** from the command line: `--initialise name=value`
- **Loop variables**: named variables that carry a start, increment and end value alongside their current value. Create them with `--looping name,start,increment,end` or at runtime with `@range`. Assigning to one updates its current value, and `@print` lists them separately from normal variables.
- **Commands**: `@print` lists all variables and loop variables; `@range name start increment end` creates or updates a loop variable.
- **Significant figures**: `--significantfigures 2..9` (default 3).
- Input validation with specific exit codes: `17` for bad usage, `6` for invalid variables, `9` for duplicate variables.

## Usage

```
./uqexpr [--significantfigures 2..9] [--looping string] [--initialise string] [inputfilename]
```

## Build

```
make
```

The Makefile links against `tinyexpr` and expects the course library paths. To build elsewhere, install tinyexpr and adjust the `-I` / `-L` flags in the Makefile.

## Implementation notes

- Written in C (`gnu99`) and compiled with `-Wall -Wextra -pedantic`.
- Variables and loop variables are held in dynamically allocated arrays (`realloc`) that are freed on exit.
- Command-line parsing, variable-name validation, and numeric validation (`strtod`) are in separate functions.

## AI assistance

Claude and Gemini was used during development.
