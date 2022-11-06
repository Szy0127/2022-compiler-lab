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
};

//must be declared in .h instead of .cc
class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */
public:
  X64Frame(temp::Label *name,std::list<bool> *f);
  Access *AllocLocal(bool escape)override;
};

} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
