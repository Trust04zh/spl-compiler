Lexical analysis and syntax analysis part for spl (SUSTech programming language) project.

- `src`: source code
- `test`: basic testcases
- `test-ex`: bonus testcases

see [report/phase1.md](report/phase1.md) for report for lexer and parser.

see [report/phase2.md](report/phase2.md) for report for semantic analyzer.

use `git checkout parser` to checkout the parser version.

## usage

make parser executable

```
make splc
```

apply `splc` parser on all `.spl` files in specified directory

```
./test.sh <directory>
```

## flags

```
// src/spl-parser-standalone.cpp
// yydebug = 1;  // uncomment this line to enable bison debug output
```

```
// src/spl-lexer-module.cpp
// #define SPL_LEXER_VERBOSE // uncomment this line to enable verbose output in lexer
```

## checklist

- [] type system & type propagation in expression
- [] symbol table
  - [] variable table
  - [] struct table
  - [] function table
- [] global scoping
  - [] bonus: local scoping
- [] bonus: function definition
- [] bonus: function overload
