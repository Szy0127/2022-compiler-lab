# Lab 3: Parsing

[Bisonc++ (Version 6.04.04) User Guide (fbb-git.gitlab.io)](https://fbb-git.gitlab.io/bisoncpp/manual/bisonc++.html)

- lvalue : lvalue [exp]

- exp : ID [exp] of exp
- exp : lvalue

存在shift/reduce

例如a[10] 与 a[10] of 1

必须推迟shift/reduce的时机直到下一个能看见of

增加一个过度的non-terminal 

- one: ID [exp]

- exp : one of exp
- exp : one

使用类型转换拿到public的成员变量进行赋值  缺点是一个没有任何用处的Var变量留在了内存里，还不能delete 因为成员变量被真实的ArrayExp使用了
