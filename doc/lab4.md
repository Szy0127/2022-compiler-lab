# Lab 4: Type Checking

nil和void处理可能有bug



type声明时不能先放nullptr 因为Look查找失败也返回nullptr 这样无法判断是是否提前声明

选择放入VoidTy



tiger.lex tiger.y在git操作的时候可能会把LF替换为CRLF  需要替换回LF 否则编译无法通过
