Compiler for spl (SUSTech programming language) project.

- `src`: source code
- `test`: basic testcases
- `test-ex`: bonus testcases

see [report/phase1.md](report/phase1.md) for report for lexer and parser.

see [report/phase2.md](report/phase2.md) for report for semantic analyzer.

see [report/phase3.md](report/phase3.md) for report for ir generator.

use `git checkout <tag>` to switch between phases.

## usage

make executable

```
make splc
```

apply `splc` executable on all `.spl` files in specified directory

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

```
// src/spl-semantic-analyzer-standalone.cpp
// yydebug = 1;  // uncomment this line to enable bison debug output
```

```
// src/spl-semantic-analyzer-standalone.cpp
#define SPL_SEMANTIC_ANALYZER_VERBOSE
// uncomment this line to enable verbose output in semantic analyzer
```

```
// src/spl-semantic-analyzer-standalone.cpp
// #define LOCAL_SCOPE  // uncomment this line to enable local scope
```

```
// src/spl-ir-generator-body.cpp
opt_ir();  // uncomment this line to enable optimization
```

```
// src/spl-ir-generator-standalone.cpp
#define SPL_IR_GENERATOR_DEBUG // uncomment this line to enable debug output
```
