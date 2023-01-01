# Lab 5: Tiger Compiler without register allocation

- caller saved:call前保存，call后恢复
- callee saved:进函数保存，出函数恢复

两种保护方式：
1. 手动添加保存和恢复的mov指令，可以利用寄存器分配合并mov节点来消除，这样做可以保证在任何情况下都充分利用15个寄存器
2. 利用活跃分析和寄存器分配，定义call的def和return的use来构造相应的冲突，使得在寄存器不够的情况下，会spill而不是破坏寄存器

- caller saved：不知道call的函数怎么用寄存器，所以没法用第一种。 
- callee saved:可以在函数最后加个空指令(或者每个return)，设置use是returnsink，这是第一种；函数自己是知道自己怎么进行寄存器分配的，所以也可以使用第二种。tiger中两种同时使用，最后效果是第二种。

实际上如果两种一起使用的话，returnsink没有作用。函数出口处returnsink(callee saved+return value)是活跃的，但是每条给callee saved定值的mov语句都会使得某个callee saved从live中移除，所以所有mov出现后，returnsink只剩下一个return value活跃，而函数最后也会mov返回值到return value，此定值也会把return value的活跃从函数体中移除。