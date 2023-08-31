.text
.globl is_prime_2
.type is_prime_2, @function
.set is_prime_2_framesize, 8
is_prime_2:
subq $8,%rsp
L57:
movq %rdi,(is_prime_2_framesize-8)(%rsp)
movq $2,%rcx
movq $2,%rax
movq %rsi,%rax
cqto
movq $2,%rdi
idivq %rdi
leaq 1(%rax),%rdi
L32:
cmpq %rdi,%rcx
jl L29
L31:
movq $1,%rax
L28:
jmp L56
L29:
movq %rsi,%rax
cqto
idivq %rcx
movq $0,%rax
cmpq %rax,%rdx
je L33
L34:
leaq 1(%rcx),%rcx
jmp L32
L33:
movq $0,%rax
jmp L28
L58:
movq $0,%rax
jmp L28
L56:


addq $8,%rsp
retq
.size is_prime_2, .-is_prime_2
.globl tigermain
.type tigermain, @function
.set tigermain_framesize, 32
tigermain:
subq $32,%rsp
L60:
movq %rbx,(tigermain_framesize-32)(%rsp)
movq %rbp,(tigermain_framesize-24)(%rsp)
movq %r12,(tigermain_framesize-16)(%rsp)
movq %r13,(tigermain_framesize-8)(%rsp)
movq $0,%r12
subq $8,%rsp
leaq L36(%rip),%rax
movq %rax,(%rsp)
callq init_list
addq $8,%rsp
movq %rax,%r13
movq $2,%rbp
movq $10000,%rbx
L40:
cmpq %rbx,%rbp
jl L37
L39:
movq %r12,%rdi
subq $8,%rsp
leaq L49(%rip),%rax
movq %rax,(%rsp)
callq print_2
addq $8,%rsp
movq $0,%rbp
movq %r13,%rdi
subq $8,%rsp
leaq L50(%rip),%rax
movq %rax,(%rsp)
callq len
addq $8,%rsp
movq %rax,%rbx
L54:
cmpq %rbx,%rbp
jl L51
L53:
movq (tigermain_framesize-32)(%rsp),%rbx
movq (tigermain_framesize-24)(%rsp),%rbp
movq (tigermain_framesize-16)(%rsp),%r12
movq (tigermain_framesize-8)(%rsp),%r13
jmp L59
L37:
leaq tigermain_framesize(%rsp),%rdi
movq %rbp,%rsi
subq $8,%rsp
leaq L41(%rip),%rax
movq %rax,(%rsp)
callq is_prime_2
addq $8,%rsp
movq $0,%rcx
cmpq %rcx,%rax
jne L46
L47:
leaq 1(%rbp),%rbp
jmp L40
L46:
leaq 1(%r12),%r12
movq $100,%rax
movq %r12,%rax
movq %r12,%rax
cqto
movq $100,%rcx
idivq %rcx
movq $0,%rax
cmpq %rax,%rdx
je L43
L44:
jmp L47
L43:
movq %r13,%rdi
movq %rbp,%rsi
subq $8,%rsp
leaq L42(%rip),%rax
movq %rax,(%rsp)
callq append
addq $8,%rsp
jmp L44
L51:
movq (%r13),%rsi
movq $8,%rax
movq $1,%rdx
movq %rbp,%rcx
addq %rdx,%rcx
imulq %rcx
movq (%rsi,%rax),%rdi
subq $8,%rsp
leaq L55(%rip),%rax
movq %rax,(%rsp)
callq print_2
addq $8,%rsp
leaq 1(%rbp),%rbp
jmp L54
L59:


addq $32,%rsp
retq
.size tigermain, .-tigermain
.section .rodata
L36:
.long 4
.string "-40 "
L41:
.long 4
.string "-40 "
L42:
.long 4
.string "-40 "
L49:
.long 4
.string "-40 "
L50:
.long 4
.string "-40 "
L55:
.long 4
.string "-40 "
