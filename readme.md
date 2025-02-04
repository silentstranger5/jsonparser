# JSON Parser

This is a JSON file parser.

## How to use

Check `main.c` for usage example

## How to build

This project does not contain any dependencies and does not rely on any additional tools. `jsonparser.c` file contains code that you can use as a library. If you compile it with `main.c` you can obtain a demo file that parses provided json file and prints its data back as a JSON string.

If you want to use CMake:

```bash
cmake -B build -S .
cmake --build build
```

You can use the program like this:

```bash
jsonparser [file.json]  # replace file.json with path to .json file
```