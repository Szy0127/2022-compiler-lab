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

### 保存指针的位置

gc是调用alloc时触发的，alloc作为一个函数调用，需要满足caller saved registers已被保存。如果alloc前有一个指针变量p存在caller saved register中，那么

1. alloc后活跃，那么发生冲突，要么不染caller saved registers的颜色，要么发生spill，此时可在栈上获取
2. alloc后不活跃，那么此时p已经变成垃圾，回收是正确的。

所以只需要看以下两个

- stack：偏移
- callee-saved register：如果存的是指针，push到栈上，然后存偏移

<img src="C:\Users\Shen\AppData\Roaming\Typora\typora-user-images\image-20221215222340742.png" alt="image-20221215222340742" style="zoom:50%;" />

### 存pointer map

每次call之前都要生成一个

<img src="C:\Users\Shen\AppData\Roaming\Typora\typora-user-images\image-20221215222454434.png" alt="image-20221215222454434" style="zoom:50%;" />

<img src="C:\Users\Shen\AppData\Roaming\Typora\typora-user-images\image-20221215222121693.png" alt="image-20221215222121693" style="zoom:50%;" />

<img src="C:\Users\Shen\AppData\Roaming\Typora\typora-user-images\image-20221215230042051.png" alt="image-20221215230042051" style="zoom:50%;" />

看不懂，可行的解决方案：每次call之前，生成一个string label，push这个string label到栈上，后面紧接着是call的retaddr

这样做可以直接找到这个string label 拿到相应的map，但是限制了所有runtime的c函数不能超过6个参数，可以后续考虑其他的解决方案

### scan

每个call对应一个map  每个map除了存偏移外再存一个framesize 根据栈大小可以一直往上找

找到栈的最后一个位置，拿到值，去gc的map拿到对应的pointer map

gc是进入调用了c函数的alloc进入的

`push rbp;mov rsp,rbp;sub rsp`

此时rbp指向retaddr,rbp+8指向调用者的栈帧，由于alloc没有超过6个参数，所以栈上就是最后放着的是指针的寄存器或者没有
