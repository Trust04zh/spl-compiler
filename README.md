Lexical analysis and syntax analysis part for spl (SUSTech programming language) project.

todo

- [x] support `int` exp
- [x] support `float` exp
- [x] support `char` exp
- [] add yyaction (for detailed error report?)
- [] recognize lexical error
- [] recognize syntax error
- [] water some examples to test lexer
- [x] support standalone lexer
- [x] support debugging lexer
- [] support standalone parser
- [] bonus: support single line comment
- [] bonus: support multi line comment
- [] add actions in lexer (see details below)


process for actions in lexer

// const value: int float char
// install in symbol table: id (id.id?)

- [] int
- [] float
- [] char
- [] id
- [] type
- [] struct
- [] if
- [] else
- [] while
- [] return
- [] dot
- [] semi
- [] comma
- [] assign
- [] lt
- [] le
- [] gt
- [] ge
- [] ne
- [] eq
- [] plus
- [] minus
- [] mul
- [] div
- [] and
- [] or
- [] not
- [] lp
- [] rp
- [] lb
- [] rb
- [] lc
- [] rc

