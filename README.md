Lexical analysis and syntax analysis part for spl (SUSTech programming language) project.

usage

make parser executable

```
make splc
```

apply `splc` parser on all `.spl` files in specified directory

```
./test.sh <directory>
```

flags

```
// src/spl-parser-standalone.cpp
// yydebug = 1;  // uncomment this line to enable bison debug output
```

```
// src/spl-lexer-module.cpp
// #define SPL_LEXER_VERBOSE // uncomment this line to enable verbose output in lexer
```

todo

- [] add our basic testcases 
- [] add bonus part (with testcases)
  - [] bonus: support single line comment
  - [] bonus: support multi line comment
- [] test lexer and parser with more testcases
- [] write report
- [] adapt to submission requirements