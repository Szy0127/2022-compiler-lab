# Lab 4: Type Checking

nil和void处理可能有bug



type声明时不能先放nullptr 因为Look查找失败也返回nullptr 这样无法判断是是否提前声明

选择放入VoidTy
