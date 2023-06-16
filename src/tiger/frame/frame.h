#ifndef TIGER_FRAME_FRAME_H_
#define TIGER_FRAME_FRAME_H_

#include <list>
#include <memory>
#include <string>

#include "tiger/frame/temp.h"
#include "tiger/translate/tree.h"
#include "tiger/codegen/assem.h"

extern bool compile_function;
namespace frame {

class RegManager {
public:
  RegManager() : temp_map_(temp::Map::Empty()) {}

  temp::Temp *GetRegister(int regno) { return regs_[regno]; }

  /**
   * Get general-purpose registers except RSI
   * NOTE: returned temp list should be in the order of calling convention
   * @return general-purpose registers
   */
  [[nodiscard]] virtual temp::TempList *Registers() = 0;

  /**
   * Get registers which can be used to hold arguments
   * NOTE: returned temp list must be in the order of calling convention
   * @return argument registers
   */
  [[nodiscard]] virtual temp::TempList *ArgRegs() = 0;

  /**
   * Get caller-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return caller-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CallerSaves() = 0;

  /**
   * Get callee-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return callee-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CalleeSaves() = 0;

  /**
   * Get return-sink registers
   * @return return-sink registers
   */
  [[nodiscard]] virtual temp::TempList *ReturnSink() = 0;

  /**
   * Get word size
   */
  [[nodiscard]] virtual int WordSize() = 0;

  [[nodiscard]] virtual temp::Temp *FramePointer() = 0;

  [[nodiscard]] virtual temp::Temp *StackPointer() = 0;

  [[nodiscard]] virtual temp::Temp *ReturnValue() = 0;

  [[nodiscard]] virtual temp::TempList *DoubleRegs() = 0;

  temp::Map *temp_map_;
protected:
  std::vector<temp::Temp *> regs_;
  std::vector<temp::Temp *> double_regs_;
};

class Access {
public:
  /* TODO: Put your lab5 code here */
  
  virtual ~Access() = default;
  virtual tree::Exp *ToExp(tree::Exp *framePtr) const = 0;

};

class Frame {
  /* TODO: Put your lab5 code here */
public:
  Frame(temp::Label *name,std::list<bool> *f):name_(name){}
  virtual Access *AllocLocal(bool escape,bool is_pointer=false,bool is_double=false) = 0;
  virtual ~Frame()=default;


  frame::Access *StaticLink(){return formals_.front();}  
  [[nodiscard]] const std::list<frame::Access *> &GetFormalList() const { return formals_; }
  [[nodiscard]] const std::list<tree::Stm*> &GetVSList() const { return view_shift_stm; }
  [[nodiscard]] const std::vector<int> &GetPointerInfo()const{return pointer_off;}

  temp::Label *name_;//for output.cc
  std::string GetLabel(){return temp::LabelFactory::LabelString(name_);}

  unsigned long GetFrameSize(){return -sp_off;}


protected:
  unsigned long sp_off{0};//sp-fp
  //actually here Stm must be MoveStm
  std::list<tree::Stm*> view_shift_stm;

  //use list<bool> to allocate list<access>
  std::list<frame::Access *> formals_;

  std::vector<int> pointer_off;//addr-fp (<0)
};

/**
 * Fragments
 */

class Frag {
public:
  virtual ~Frag() = default;

  enum OutputPhase {
    Proc,
    String,
    Double,
  };

  /**
   *Generate assembly for main program
   * @param out FILE object for output assembly file
   */
  virtual void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const = 0;
};

class StringFrag : public Frag {
public:
  temp::Label *label_;
  std::string str_;

  StringFrag(temp::Label *label, std::string str)
      : label_(label), str_(std::move(str)) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
};
class DoubleFrag : public Frag {
public:
  temp::Label *label_;
  double val_;

  DoubleFrag(temp::Label *label, double val)
      : label_(label), val_(val) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
};

class ProcFrag : public Frag {
public:
  tree::Stm *body_;
  Frame *frame_;

  ProcFrag(tree::Stm *body, Frame *frame) : body_(body), frame_(frame) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
};

class Frags {
public:
  Frags() = default;
  void PushBack(Frag *frag) { if(compile_function)frags_.emplace_back(frag); }
  const std::list<Frag*> &GetList() { return frags_; }

private:
  std::list<Frag*> frags_;
};

/* TODO: Put your lab5 code here */
tree::Exp *externalCall(std::string s,tree::ExpList *args,StringFrag *pointer_map,int arg_in_stack=0);
// tree::Exp *staticLink(tr::Level *level_now,tr::Level *level_target);
std::list<tree::Stm*> ProcEntryExit1(frame::Frame *frame, tree::Stm *func_body);
assem::Proc *ProcEntryExit3(frame::Frame *frame,assem::InstrList *instr_list);

assem::InstrList* ProcEntryExit2(assem::InstrList*body);
} // namespace frame

#endif