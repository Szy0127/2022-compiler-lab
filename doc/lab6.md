# Lab 6: Register Allocation

可以把output.cc的need_ra改成false，这样可以看到正确的txxx的代码

可以不进行src=dst的move的删除，这样两份代码除了寄存器分配是一模一样的，便于找问题

interfgraph处输出冲突图，手动分析是否是冲突图生成错了

如果冲突图无误，而最后程序出问题，就是寄存器分配给有冲突的两个txxx分配了同一个寄存器

可以在assigncolor后通过Adj和color来验证

之后再assigncolor的函数中，输出所有染色的过程，大概率是合并出错，可以去combine和addedge看是不是对应的冲突边没有加上



预着色节点是无法simplify的，所以degree需要设为无穷

- 实现合并move
- 启发式selectspill:优先不考虑由rewrite引入的，优先选择度数高的
- spill：如果没有活跃冲突，可以多个共用一个frame slot