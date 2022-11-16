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

```
// src/spl-semantic-analyzer-standalone.cpp
#define SPL_SEMANTIC_ANALYZER_VERBOSE  // uncomment this line to enable verbose output in semantic analyzer

```
// src/spl-semantic-analyzer-standalone.cpp
// yydebug = 1;  // uncomment this line to enable bison debug output
```

```
// src/spl-semantic-analyzer-standalone.cpp
#define SPL_SEMANTIC_ANALYZER_VERBOSE  // uncomment this line to enable verbose output in semantic analyzer
```

## checklist

**anything for struct not implemented yet**

- [] type system & type propagation in expression
- [] error report
  - [x] 1
  - [] 2
  - [x] 3
  - [] 4
  - [x] 5
  - [x] 6
  - [x] 7
  - [] 8
  - [] 9
  - [] 10
  - [] 11
  - [] 12
  - [] 13
  - [] 14
  - [] 15
  - [x] 21
- [] symbol table
  - [x] variable table
  - [] struct table
  - [x] function table
- [] global scoping
  - [] bonus: local scoping
- [] bonus: function declaration
- [] bonus: function overload
