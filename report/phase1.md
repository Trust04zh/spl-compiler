根据project文档，基础功能部分不赘述，the code speaks。

## 错误恢复

## Bonus功能

### 单行注释与多行注释

`YYSTATE`为默认的`INITIAL`时，lexer读到`//`时，设置`YYSTATE`为`SINGLE_LINE_COMMENT`，指示单行注释状态，直到遇到换行符或EOF；读到`/*`时，设置`YYSTATE`为`SINGLE_LINE_COMMENT`，指示多行注释状态，直到遇到`*/`。注释结束时`YYSTATE`置为`INITIAL`。

读到EOF时，如果`YYSTATE`为`MULTI_LINE_COMMENT`，说明多行注释未闭合，报错。

在我们的设计中，出现注释相关的词法错误时不尝试任何错误恢复，立即终止程序。

样例输入输出见`test-ex`下的1-4组。

一点闲扯，此前考虑如何用`input`（cpp下是`yyinput`）和`unput`做多行注释(虽然最后没这么写有一个原因是觉得用start condition比较优雅)，发现需要判一下读到EOF，遂用`(c = input()) == -1`，然后发现一个拉高血压的玩意，[这里](https://github.com/westes/flex/issues/448)提到flex 2.6.4版本下`input`读到EOF居然是返回0，不知道为什么改成这样也没改回来，那么要判定读到EOF就要和0比，代价就是注释里不能放`\x00`。~~虽然不会无聊到往里塞`\x00`，但不让放就是另一回事了~~

### For循环

就，，加产生式。

样例输入输出见`test-ex`下的5-6组。

