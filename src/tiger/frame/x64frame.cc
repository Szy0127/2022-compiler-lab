#include "tiger/frame/x64frame.h"

extern frame::RegManager *reg_manager;

namespace frame {
/* TODO: Put your lab5 code here */
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

class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */
};
/* TODO: Put your lab5 code here */

X64RegManager::X64RegManager(){
  auto regs = std::vector<std::string>{
    "rax","rbx","rcx","rdx","rsi","rdi","rbp","rsp",
    "r8","r9","r10","r11","r12","r13","r14","r15"
  };
  for(const auto &reg:regs){
    auto temp = temp::TempFactory::NewTemp();
    regs_.push_back(temp);
    temp_map_->Enter(temp,new std::string(reg));
  }
}

temp::TempList *X64RegManager::Registers() {
  auto templist = new temp::TempList();
  for(const auto& reg:regs_){
    templist->Append(reg);
  }
  return templist;
}

temp::TempList *X64RegManager::ArgRegs() {
  return new temp::TempList{
    regs_[5], regs_[4], regs_[3], regs_[2], regs_[8], regs_[9]
  };
}

temp::TempList *X64RegManager::CallerSaves() {
  return new temp::TempList{
    regs_[5], regs_[4], regs_[3], regs_[2], regs_[8], regs_[9],regs_[0],regs_[10],regs_[11]
  };
}

temp::TempList *X64RegManager::CalleeSaves() {
  return new temp::TempList{
    regs_[1], regs_[6], regs_[12], regs_[13], regs_[14], regs_[15]
  };
}

temp::TempList *X64RegManager::ReturnSink() {
  return new temp::TempList{};//??what is sink?
}

int X64RegManager::WordSize() {
  return 8;
}

temp::Temp *X64RegManager::FramePointer() {
  return regs_[6];
}

temp::Temp *X64RegManager::StackPointer() {
  return regs_[7];
}

temp::Temp *X64RegManager::ReturnValue() {
  return regs_[0];
}


} // namespace frame