Lexical analysis and syntax analysis part for spl (SUSTech programming language) project.

- `src`: source code
- `test`: basic testcases
- `test-ex`: bonus testcases

see [report/phase1.md](report/phase1.md) for report for course project submission.

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

- [x] add our basic testcases 
- [x] support error detection
- [x] add bonus part (with testcases)
  - [x] bonus: support single line comment
  - [x] bonus: support multi line comment
  - [x] testcases
- [x] test lexer and parser with more testcases
- [] write report
- [] adapt to submission requirements
