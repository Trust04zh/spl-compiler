根据project文档，基础功能部分不赘述，the code speaks。

## 错误恢复

对于词法分析中遇到的错误，若为格式不合法的INT或ID等，我们选择打印报错信息后仍然向bison输出对应的终结符。若为无法匹配的字符，则打印报错信息，并把它和它之后的所有可打印字符（到第一个空格或不可打印字符为止）全部作为ID。

对于语法分析中的错误，我们简单对如下几种错误做了错误恢复：
- 缺失`}`（这一条包括定义不在语句块开头的情况。当在上一个CompSt没有结束时遇到定义时，bison会认为这是下一个CompSt的开始，而CompSt中间应该有`}{`，所以会报出缺失`}`的错误。）
- else没有对应if
- 缺失分号

## Bonus功能

### 单行注释与多行注释

`YYSTATE`为默认的`INITIAL`时，lexer读到`//`时，设置`YYSTATE`为`SINGLE_LINE_COMMENT`，指示单行注释状态，直到遇到换行符或EOF；读到`/*`时，设置`YYSTATE`为`SINGLE_LINE_COMMENT`，指示多行注释状态，直到遇到`*/`。注释结束时`YYSTATE`置为`INITIAL`。

读到EOF时，如果`YYSTATE`为`MULTI_LINE_COMMENT`，说明多行注释未闭合，报错。

样例输入输出见`test-ex`下的1-4组。

一点闲扯，此前考虑如何用`input`（cpp下是`yyinput`）和`unput`做多行注释(虽然最后没这么写有一个原因是觉得用start condition比较优雅)，发现需要判一下读到EOF，遂用`(c = input()) == -1`，然后发现一个拉高血压的玩意，[这里](https://github.com/westes/flex/issues/448)提到flex 2.6.4版本下`input`读到EOF居然是返回0，不知道为什么改成这样也没改回来，那么要判定读到EOF就要和0比，代价就是注释里不能放`\x00`。~~虽然不会无聊到往里塞`\x00`，但不让放就是另一回事了~~

### For循环

就，，加产生式。

样例输入输出见`test-ex`下的5-6组。

### Include

Include的实现比较暴力，只是简单的匹配`#include<`，然后把下一个`>`之间的所有字符都当作文件名读入。读取文件后，把整个文件直接反向逐字符的用`unput()`放到flex里面。

这样做的效率应该不会很高，并且无法处理目录问题，但在flex里可能也只能这样了。更好的方式应该是单独写一个preprocessor处理include和宏定义，之后有时间可能会重写这一部分。
