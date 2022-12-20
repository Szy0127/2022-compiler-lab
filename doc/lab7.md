# Lab 7: Garbage Collection

alloc调GC模块提供的操作

使用复制式收集

## 类型信息

gc是runtime alloc是会到gc来处理的，所以这部分可以直接在c语言实现，不需要存到汇编

每个record/array需要descriptor，在alloc时使用`map<addr,Desc>`存储

Desc分为DescRecord与DescArray，存储size及bitmap

需要修改translate传递的参数

## 找root

### 识别指针变量

- 修改Temp类型，加入是否存指针的信息
- stack slot 标记是否是指针
- 寄存器分配也可以有信息传递

为了传递信息，需要给tree和assem的call都增加frag的地址信息（由于不想增加一个CallInstr破坏原先大部分的逻辑代码，给OperInstr加两个变量保存）

translate层，生成stringfrag，加入所有stack的信息

regalloc的rewrite 可能再加入stack

regalloc成功最后，是汇编生成的终点，这时候可以随意调整汇编代码，需要的信息想办法从translate、codegen传下来即可

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

### pointermap 结构设计

栈布置如下

| stack             | info in pointer_map |
| ----------------- | ------------------- |
| escape local var1 | -8                  |
| escape local var2 | -16                 |
| rbx               | 32                  |
| rdi               | 24                  |
| arg8              |                     |
| arg7              |                     |
| pointer_map_label |                     |
| retaddr           | <--rsp              |

pointermap = frame_size + off1+off2+...+offn，offi = +/-8*m

translate和regalloc过程都有一个frame的 拿framesize很容易

为了便于判断是否达到tiger的栈底，把main层的大小加上负号

大于6个参数放在栈上很难处理，因为f call g的参数信息是存在f里的而不是g里，所以只能记入f的framesize，而拿到f call g的pointermap，需要跳过这些参数，然后再拿reg pointer value

以下两个方案

1. 参数紧跟着pointermap，callee每次固定从rsp+16开始拿数据，但需要把reg pointer插入到参数之前并记录reg pointer 在栈上的偏移，需要在translate的callexp一路向下传递每个call对应的参数个数，然后regalloc找到合适的插入位置。
2. reg pointer value紧跟着pointermap，这样只需要记录pointermap上有几个值即可，scan时找起来也方便。但callee每次取值需要变化，可以在codegen时assem作特殊标记，regalloc最后根据实际情况修改取参数的位置。

### 问题

alloc的逻辑是frame->AllocLocal  call alloc_record  set values

而alloc本身需要知道指针信息，这时候frame已经认为提前为alloc返回值准备好的参数是存指针的，但是此时指针值还未被mov进来

需要在完成每次alloc之后再修改相应的pointer_map，而不能在每次一开始就改

每个frame中stack slot的pointer信息 对于这个frame中每个call是不同的

太麻烦了，目前想到的解决方案如下

- stack：在每个frame开始先把所有的stack是pointer的位置赋0值，且只能在rewrite之后做。
- register：live-in = use + (out-def)，call后`movq %rax,txxx` 由于txxx每次都是新的变量，因此只在这句首次出现且是def，因此txxx向上的所有部分都不会活跃，因此不存在此问题

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

我们记录的framesize是localvariable+大于6的参数+是指针的寄存器，并没有加上每次call被push的retaddr，因此在scan时需要每次手动加一个wordsize

[C语言进阶——内联汇编_Li-Yongjun的博客-CSDN博客_c语言内联汇编](https://blog.csdn.net/lyndon_li/article/details/118471845)

## 回收

需要更改程序变量存储的指针值，因此不能只拿指针值，而是要拿指针变量自己的地址

stack中的直接被修改，register的较麻烦。

```c++
f(){
	int *a;//r13
	g(a);//r13->rsi
}
g(a){
	a//rsi->r12
    alloc();
}
```

此时move的优化保证了f和g中对于同一个变量绝不会使用两个不同的寄存器，因此要么是同一个寄存器，要么有一个会出现在栈上，只要出现在栈上那就不用担心值无法被修改。

因此只要在alloc这个函数之后把栈上对应位置的数据写回对应寄存器即可，不需要所有函数都做一遍。

由于函数调用只有rax有影响，而rax又是caller saved 因此没有问题。
