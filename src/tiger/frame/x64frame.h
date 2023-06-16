//
// Created by wzl on 2021/10/12.
//

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include "tiger/frame/frame.h"

namespace frame {
class X64RegManager : public RegManager {
  /* TODO: Put your lab5 code here */
public:
  X64RegManager();

  temp::TempList *Registers() override;

  temp::TempList *ArgRegs() override;

  temp::TempList *CallerSaves() override;

  temp::TempList *CalleeSaves() override;

  temp::TempList *ReturnSink() override;

  int WordSize() override;

  temp::Temp *FramePointer() override;

  temp::Temp *StackPointer() override;

  temp::Temp *ReturnValue() override;
  temp::TempList *DoubleRegs() override;
};

//must be declared in .h instead of .cc
class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */
public:
  X64Frame(temp::Label *name,std::list<bool> *f,std::list<bool> *is_pointer);
  Access *AllocLocal(bool escape,bool is_pointer=false,bool is_double=false)override;

};


//must be declared in .h instead of .cc
class InFrameAccess : public Access {
public:
  int offset;

  explicit InFrameAccess(int offset) : offset(offset) {}
  /* TODO: Put your lab5 code here */
  tree::Exp *ToExp(tree::Exp *framePtr) const override {
    return new tree::MemExp(
      new tree::BinopExp(
        tree::PLUS_OP,
        framePtr,
        new tree::ConstExp(offset)
      )
    );
  }

};


class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}
  /* TODO: Put your lab5 code here */
  tree::Exp *ToExp(tree::Exp *framePtr) const override {
    return new tree::TempExp(reg);
  }

};


} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
