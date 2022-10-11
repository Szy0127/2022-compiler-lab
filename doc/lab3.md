# Lab 3: Parsing

[Bisonc++ (Version 6.04.04) User Guide (fbb-git.gitlab.io)](https://fbb-git.gitlab.io/bisoncpp/manual/bisonc++.html)

## shift/reduce conflict

- lvalue : lvalue [exp]

- exp : ID [exp] of exp
- exp : lvalue

存在shift/reduce  例如a[10] 与 a[10] of 1

可以设置ID与[的优先级，但这样每次都会执行shift或每次都会执行reduce

所以必须推迟shift/reduce的时机直到下一个能看见of，也就是说原先的文法并不是LR(1)

增加一个过度的non-terminal 

- one: ID [exp]

- exp : one of exp
- exp : one

使用类型转换拿到public的成员变量进行赋值  缺点是一个没有任何用处的Var变量留在了内存里，还不能delete 因为成员变量被真实的ArrayExp使用了

增加lvalue:one 避免其他情况的错误

## warnings

一些没有定义优先级导致的conflict 例如负号并没有指定优先级

以及没有使用的non-terminal和terminal 例如break

虽然过了所有测试 但并不是完整的tiger语言

等日后碰到了再补充

```shell
[Warning] 75 Shift/Reduce conflict(s)
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 60 (tiger.y, line 156): shifts at VAR, VAR, TYPE, FUNCTION, FUNCTION,    rule 57 (tiger.y, line 151): shifts at VAR, VAR, TYPE, FUNCTION, FUNCTION,
   rule 4 (tiger.y, line 72): shifts at LBRACK,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 60 (tiger.y, line 156): shifts at VAR, VAR, TYPE, FUNCTION, FUNCTION,    rule 57 (tiger.y, line 151): shifts at VAR, VAR, TYPE, FUNCTION, FUNCTION,
   rule 57 (tiger.y, line 151): shifts at TYPE,
   rule 60 (tiger.y, line 156): shifts at FUNCTION, FUNCTION,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at RPAREN, MINUS,    rule 42 (tiger.y, line 124): shifts at RPAREN,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 44 (tiger.y, line 128): shifts at RPAREN,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 60 (tiger.y, line 156): shifts at VAR, VAR, TYPE, FUNCTION, FUNCTION,    rule 57 (tiger.y, line 151): shifts at VAR, VAR, TYPE, FUNCTION, FUNCTION,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 16 (tiger.y, line 95): shifts at ELSE,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
   rule 37 (tiger.y, line 116): shifts at MINUS,
[Warning] 39 Reduce/Reduce conflict(s)
   keeping rule 57 (tiger.y, line 151), dropping
        rule 60 (tiger.y, line 156)
   keeping rule 5 (tiger.y, line 73), dropping
        rule 9 (tiger.y, line 88)
   keeping rule 57 (tiger.y, line 151), dropping
        rule 60 (tiger.y, line 156)
   keeping rule 50 (tiger.y, line 137), dropping
        rule 57 (tiger.y, line 151)
        rule 60 (tiger.y, line 156)
   keeping rule 57 (tiger.y, line 151), dropping
        rule 60 (tiger.y, line 156)
   keeping rule 50 (tiger.y, line 137), dropping
        rule 57 (tiger.y, line 151)
        rule 60 (tiger.y, line 156)
   keeping rule 37 (tiger.y, line 116), dropping
        rule 47 (tiger.y, line 132)
   keeping rule 37 (tiger.y, line 116), dropping
        rule 42 (tiger.y, line 124)
   keeping rule 37 (tiger.y, line 116), dropping
        rule 47 (tiger.y, line 132)
[Warning] Terminal symbol(s) not used in productions:
[Warning]    283: BREAK
[Warning]    301: expseq
[Warning]    308: oneormore
[Warning] Non-terminal symbol(s) not used in productions:
[Warning]   nonemptyactuals
[Warning]   rec_nonempty
[Warning] Unused production rule(s):
[Warning]   38: nonemptyactuals (COMMA) ->  exp COMMA actuals
[Warning]   39: nonemptyactuals ->  exp
[Warning]   72: rec_nonempty (COMMA) ->  rec_one COMMA rec
[Warning]   73: rec_nonempty ->  rec_one

```

