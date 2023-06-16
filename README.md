# python编译器

```python
def is_prime(n):
	for i in range(2,n//2+1):
		if n % i == 0:
			return False
	return True
count = 0
prime_list = []
for i in range(2,10000):
	if is_prime(i):
		count += 1
		if count % 100 == 0:
			prime_list.append(i)
print(count)
for i in range(len(prime_list)):
	print(prime_list[i])



def eq(a,b):
	if a==b:
		return "eq"
	else:
		return "not eq"


print(eq(1,1))
print(eq(1,2))
print(eq("aa","aa")) 
print(eq("aa","ab")) 
a = 1+3
print(a)
print("hello world")


a = 3.4
print(a) # 3.400000
```

## 编译

runtime生成.so

`sudo g++ -m64 -shared -fPIC ../src/tiger/runtime/runtime.cc ../src/tiger/runtime/gc/heap/derived_heap.cc -o libtigerruntime.so`

`sudo g++ -m64 -no-pie -Wl,-rpath,. test.py.tig.s -o test.out -L. -ltigerruntime -Wl,--wrap=getchar`

或者直接一起编译

`sudo g++ -Wl,--wrap,getchar -m64 test.py.tig.s ../src/tiger/runtime/runtime.cc ../src/tiger/runtime/gc/heap/derived_heap.cc -o test.out`

比python快10倍

## 分析

- python是动态语言 解释执行 不需要先定义再使用

  `a = 1 if b else '1'`

  这种情况下无法在编译期知道a的类型

  但是实际上可以ban掉这种情况，codon也是这样做的

  编译器在发生这个语句的情况下判断两个类型是否相等

  每次赋值修改env中的类型

  使用时一定会先定义 函数的情况比较特殊 codon会根据每种不同的函数调用生成不同的函数，类似重载

  同理由return决定函数返回值 以下这种也会在编译器被ban掉

  ```python
  def f(a):
  	return 1 if a else '1'
  ```

  

- python使用缩进，这不满足LR0文法？

  只能通过preprocessor 用程序解决

  相当于做python->tiger 的翻译器

## 目标

- 不考虑鲁棒性，只考虑用户完全按照正常语法使用

## 已完成

1. 取消变量定义  一切使用变量赋值（=）

   semant和translate在第一次碰到未定义的变量时不报错，而是加入symbol table

2. 增加+= -= 等操作，类似赋值，在tiger.y通过var构造exp

   增加MOD 类似DIV

   增加// 目前和/一样 只需要修改lex

   把&和|改成and or，实际上应该增加& | 为位运算的情况

   增加not 处理类似负号 逻辑类似and or

3. 增加`for i in range():`循环，将do改为冒号，不影响，在前端通过多个cfg补充默认值0

   把translate中for的LE改成LT 右端开区间

4. 把printi改成print，需要改runtime和env

5. 去掉函数参数和返回值需要带的类型，把semant translate涉及到的类型全部去掉

   所有涉及到类型的部分

   - 判断指针，用于垃圾回收
   - 判断类型，用于不同逻辑，实际上op只支持了整形
   - 类型分析，提前报错

6. 增加returnexp 逻辑类似breakexp，functiondec在最后增加一个label return直接跳 

   跳到函数mov rax之后，但是return这句需要先mov rax再跳

   需要给所有translate加一个参数 区分break_label和return_label 防止for循环return返回for下一句的bug

7. 前端增加True False 当作int处理

8. 修改IF前端 由于根据缩进处理的时候，认为缩进=代码块结束=需要补分号 而if else本身是一个语句 所以这里不妨处理为if else中间需要分号

   增加exp if exp else exp的三元组

9. while do改为分号

10. 取消let in end 增加functionexp直接包含一个functiondec

   let in end设置了scope，但实际上每个function也自带了scope 不需要做改动 

   functionexp只是作为functiondec的一个包装 调用成员functiondec相关函数即可

   这样修改的坏处是把dec作为了exp处理 导致`a = def f()`合法

11. preprocessor根据代码缩进判断代码块 每块前后括号 中间分号

    用getline读一行 eof判断结尾 但是都成功也会导致eof推出循环 所以需要对最后一次处理

12. 分析每种函数调用，生成不同函数，加上参数类型，这样可以做到编译期知道类型，相当于重载

    callexp本身就知道所有arg的类型  此时可以根据类型构造一个functiondec 真正的function是带参数类型的

    ast的解析顺序：函数定义->函数调用

    但是这里必须先在函数调用时分析类型再函数定义，函数定义->函数调用->函数定义 不可行

    如果在preprocessor分析类型再生成多套函数定义代码 太复杂 且无法复用代码

    因此解决方案是带一个flag 跑两边translate  第二遍才frag->push

    |        | functiondec                             | callexp                                |
    | ------ | --------------------------------------- | -------------------------------------- |
    | 第一遍 | 不编译函数体 symbol为f                  | 把所有函数参数序列放入一个集合 记为f_i |
    | 第二遍 | 集合中取出所有f_i 生成不同参数类型 编译 | 根据参数类型在集合中找到名字f_i        |

    table的look和enter必须用symbol作为key 但是symbol配合bison一起用 没法自己构造 如果用另外的数据结构存 无法使用scope 只能改funcentry

    即不用多个funcentry 而是一个entry共享多个重载函数

13. 为了让runtime的函数也支持不同类型，直接在runtime中写_xxx的版本

    重载的实现也是在编译期完成的，达到汇编层面已经修改了函数名 对于print既是变长参数又是不限类型 无法做到 只能手动写几个常用的情况

14. 函数的返回值类型很重要，由于返回值类型完全由return决定，因此在所有translate函数中加入当前所在def函数的funentry的result_ty的地址，由returnexp在translate时修改。

15. 创建空列表，把List当Array复用之前的代码 修改init_list 用第一个位置存容量和当前大小 数组真实位置+8byte

    增加len和append

    append会修改数组指针(realloc)  怎么办？  目前只能固定大小
16. 加入double，支持assign print add
    后面修改binop的codgen补全所有运算然后加入和整数的互转
## 待完成

1. 修改for 全改为for i in  list

   range 改为list 增加rangeexp返回listty

2. 支持元组，把代码块()改为{} 类似record还是list？

3. 用迭代器抽象 for支持元组和list

4. 支持float

   1. 前端增加小数token 增加Double对象 完成

   2. temp增加bool表示是否用xmm寄存器

   3. 函数根据类型判断传参和返回值类型

   4. 加减乘除 常数 改代码

   5. 冲突图 如果两个temp类型不一样 不冲突

   6. 寄存器分配 按类型分配 xmm一共16个

      csapp带v前缀的 用法不太一样 支持3个operator 但是不带v的兼容之前的用法

      三个operator对寄存器分配冲击有点大

   ```assembly
   movsd %xmm0,(%rdi)
   movapd %xmm0, %xmm1
   cvttsd2siq  %xmm0, %rax
   cvtsi2sdq       %rdx, %xmm0
   addsd %xmm0,%xmm1
   subsd mulsd divsd
   comisd (%rdi),%xmm0
   
   
   mulsd .abcde(%rip),xxx
   .abcde
   .long 1243451423
   .long 12313541345
   ```

   

5. if else elif

6. 重写semant 编译器检查






## CI/CD

自己的服务器上用二进制装一个gitlab-runner

执行器为docker，镜像为ipadsse302/tigerlabs_score_env

用ipads的gitlab仓库

服务器配置vpn，为了让容器没有网络问题，修改config.toml加入`network_mode='host'`

2核2G的跑gitlab-runner+编译似乎跑不下来，升级4G内存

正常来说应该和python执行结果比较，但是涉及到数据运算可能会出现有无后缀.0的不一样结果，需要和提前设置好的答案比较

## codon



`~/.codon/bin/codon build -release -exe calc.py`

```shell
ubuntu@VM-4-10-ubuntu:~/python-compiler$ cat calc.py
def is_odd(n):
        return n%2
s = 0
for i in range(10000000):
        s += is_odd(i) + (1 if i > 1000000 else 0) + (1 if i%2 else -1)
print(s)
ubuntu@VM-4-10-ubuntu:~/python-compiler$ time python3 calc.py
13999999

real    0m2.404s
user    0m2.395s
sys     0m0.008s
ubuntu@VM-4-10-ubuntu:~/python-compiler$ sudo ./tiger-compiler calc.py
ubuntu@VM-4-10-ubuntu:~/python-compiler$ sudo g++ -m64 -no-pie -Wl,-rpath,. final_calc.py.s -o test.out -L. -ltigerruntime -Wl,--wrap=getchar
ubuntu@VM-4-10-ubuntu:~/python-compiler$ time ./test.out
13999999

real    0m0.211s
user    0m0.211s
sys     0m0.000s
ubuntu@VM-4-10-ubuntu:~/python-compiler$ ~/.codon/bin/codon build -release -exe calc.py
ubuntu@VM-4-10-ubuntu:~/python-compiler$ time ./calc
13999999

real    0m0.015s
user    0m0.015s
sys     0m0.000s
```



为什么这么快？ 编译优化？

```shell
ubuntu@VM-4-10-ubuntu:~/python-compiler$ time python3 prime.py
9592

real    0m11.963s
user    0m11.954s
sys     0m0.008s
ubuntu@VM-4-10-ubuntu:~/python-compiler$ sudo ./tiger-compiler prime.py
ubuntu@VM-4-10-ubuntu:~/python-compiler$ sudo g++ -m64 -no-pie -Wl,-rpath,. final_prime.py.s -o prime -L. -ltigerruntime -Wl,--wrap=getchar
ubuntu@VM-4-10-ubuntu:~/python-compiler$ time ./prime
9592

real    0m2.140s
user    0m2.139s
sys     0m0.000s
ubuntu@VM-4-10-ubuntu:~/python-compiler$ ~/.codon/bin/codon build -release -exe prime.py
ubuntu@VM-4-10-ubuntu:~/python-compiler$ time ./prime
9592

real    0m1.896s
user    0m1.876s
sys     0m0.004s

```

这里差不多

在不同物理机上表现相对稳定，2.1-2.5s 而python11-22s，codon有环境变量的问题