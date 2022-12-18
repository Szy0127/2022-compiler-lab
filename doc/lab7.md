# Lab 7: Garbage Collection

alloc调GC模块提供的操作

使用复制式收集

## 类型信息

gc是runtime alloc是会到gc来处理的，所以这部分可以直接在c语言实现，不需要存到汇编

每个record/array需要descriptor，在alloc时使用`map<addr,Desc>`存储

Desc分为DescRecord与DescArray，存储size及bitmap

需要修改translate传递的参数

## 找root

### 识别指针

- 修改Temp类型，加入是否存指针的信息
- stack slot 标记是否是指针
- 寄存器分配也可以有信息传递

为了传递信息，需要给tree和assem的call都增加frag的地址信息

translate层，生成stringfrag，加入所有stack的信息

regalloc的rewrite 可能再加入stack

regalloc成功最后，遍历instr 所有call之前把指针的temp入栈，并修改stringfrag的string

### 保存指针的位置

gc是调用alloc时触发的，alloc作为一个函数调用，需要满足caller saved registers已被保存。如果alloc前有一个指针变量p存在caller saved register中，那么

1. alloc后活跃，那么发生冲突，要么不染caller saved registers的颜色，要么发生spill，此时可在栈上获取
2. alloc后不活跃，那么此时p已经变成垃圾，回收是正确的。

```c++
void f(){
	int *a;
    gen_pointer_map();
    g(a);
}
```

这种情况下，由于a在调用g之后不活跃，所以a可以染caller saved register，此时并不会出现在f call g的pointer map中，之后a是否作为root由g管理。

对于g，如果在某一alloc后需要用a这个变量，那么a(rdi)与caller saved registers冲突，要么存在栈上，要么存在callee saved register中，无论哪种都会出现在g的pointer map中。如果任意alloc后不用这个变量，那对于g来说a也不活跃，可以回收。

所以只需要看以下两个

- stack：偏移
- callee-saved register：如果存的是指针，push到栈上，然后存偏移

<img src="C:\Users\Shen\AppData\Roaming\Typora\typora-user-images\image-20221215222340742.png" alt="image-20221215222340742" style="zoom:50%;" />

如何在每个call时知道此时的callee saved registers存的是不是指针？

1. 每个call可能由不同的地方jmp过来，call之前寄存器状态不确定
2. 寄存器分配只知道txxx转为了哪个机器寄存器，而很难知道机器寄存器是由哪里转来的

使用call语句的live-in可同时解决上面两个问题

1. 每个call的live-in确定，不需要看所有的callee saved registers，有些值可能不活跃了但仍在callee saved registers里还没被覆盖，此时不作为root是复合语义的
2. live-in拿到的是txxx 可同时有txxx和机器寄存器的信息

regalloc最后再进行一遍活跃分析，把call函数的live-in的寄存器取出，如果着色为callee saved 则放入栈上

| stack             | info in pointer_map |
| ----------------- | ------------------- |
| escape local var1 | 8                   |
| escape local var2 | 16                  |
| arg8              |                     |
| arg7              |                     |
| rbx               |                     |
| rdi               |                     |
| pointer_map_label |                     |
| retaddr           | rsp                 |

最后记录一个数值表示需要检查的寄存器个数，例如2 就找 rsp+16 rsp+32

frame_size + off1+off2+...+n(reg)

off = 8m

translate和regalloc过程都有一个frame的 拿framesize很容易 需要在codegen手动加上参数导致的栈增长的部分

### 存pointer map

每次call之前都要生成一个

<img src="C:\Users\Shen\AppData\Roaming\Typora\typora-user-images\image-20221215222454434.png" alt="image-20221215222454434" style="zoom:50%;" />

<img src="C:\Users\Shen\AppData\Roaming\Typora\typora-user-images\image-20221215222121693.png" alt="image-20221215222121693" style="zoom:50%;" />

<img src="C:\Users\Shen\AppData\Roaming\Typora\typora-user-images\image-20221215230042051.png" alt="image-20221215230042051" style="zoom:50%;" />

看不懂，可行的解决方案：每次call之前，生成一个string label，push这个string label到栈上，后面紧接着是call的retaddr

这样做可以直接找到这个string label 拿到相应的map，但是限制了所有runtime的c函数不能超过6个参数，可以后续考虑其他的解决方案

### scan

每个call对应一个map  每个map除了存偏移外再存一个framesize 根据栈大小可以一直往上找

这里的逻辑和staticlink不同，因为staticlink是直接存的地址，而这里的framesize需要加上大于6个的参数在栈上占用的大小。如果修改栈调整的逻辑比较麻烦，因此这里只能手动加。真实的情况是结合`push rbp,movq rsp,rbp` `push` `leave`

的操作来实现的，但是我们并不支持这样实现。

找到栈的最后一个位置，拿到值，去gc的map拿到对应的pointer map

gc是进入调用了c函数的alloc进入的

`push rbp;mov rsp,rbp;sub rsp`

此时rbp指向retaddr,rbp+8指向调用者的栈帧，由于alloc没有超过6个参数，所以栈上就是最后放着的是指针的寄存器或者没有
