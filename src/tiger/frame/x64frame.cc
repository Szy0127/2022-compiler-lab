#include "tiger/frame/x64frame.h"
#include <sstream>
#include <list>
extern frame::RegManager *reg_manager;

namespace frame {
/* TODO: Put your lab5 code here */
X64RegManager::X64RegManager(){
  auto regs = std::vector<std::string>{
    //should add % because Format in assem.cc dont add
    "%rax","%rbx","%rcx","%rdx","%rsi","%rdi","%rbp","%rsp",
    "%r8","%r9","%r10","%r11","%r12","%r13","%r14","%r15"
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
  //in order
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
  auto temp_list = CalleeSaves();
  temp_list->Append(StackPointer());
  temp_list->Append(ReturnValue());
  return temp_list;
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

//if put in .h  multiple definition
tree::Exp *externalCall(std::string s,tree::ExpList *args,temp::Label *pointer_map_label){
  return new tree::CallExp(
    new tree::NameExp(temp::LabelFactory::NamedLabel(s)),
    args,
    pointer_map_label);
}
// tree::Exp *staticLink(tr::Level *level_now,tr::Level *level_target){
//   auto framePtr = new tree::TempExp(reg_manager->FramePointer());
//   while(level_now != level_target){
//     //only main dont have static link formal
//     framePtr = l->frame_->StaticLink()->ToExp(framePtr);
//     level_now = level_now->parent_;
//   }
//   return framePtr;
// }


/* TODO: Put your lab5 code here */
//NewFrame
X64Frame::X64Frame(temp::Label *name,std::list<bool> *f):Frame(name,f){
  if(!f){
    return;
  }
  // although some escape params should be allocated in frame instead of register
  // we should follow the x86 64 rules to move frame param to reg e.g. rdi rsi
  for(const auto &escape:*f){
    formals_.push_back(AllocLocal(escape));
  }
  auto reg_list = reg_manager->ArgRegs()->GetList();
  auto max_index = reg_list.size();
  auto reg_it = reg_list.begin();
  auto frame_ptr = new tree::TempExp(reg_manager->FramePointer());
  auto word_size = reg_manager->WordSize();
  //here contains static link , in rdi
  auto i = 0;
  for(const auto&formal:formals_){
    tree::MoveStm* move = nullptr;
    if(i < max_index){
      move = new tree::MoveStm(
        formal->ToExp(frame_ptr),
        new tree::TempExp(*reg_it)
      );
    }else{
      /*
          param 7th
          pointer_map (for gc)
          ret addr
                  <-- frame ptr       
          local var 
      */
      move = new tree::MoveStm(
        formal->ToExp(frame_ptr),
        new tree::MemExp(
          new tree::BinopExp(
            tree::PLUS_OP,
            frame_ptr,
            new tree::ConstExp(word_size*(i-4))
          )
        )
      );
    }
    i++;
    reg_it++;
    view_shift_stm.push_back(move);
  }
  
}

std::list<tree::Stm*> ProcEntryExit1(frame::Frame *frame, tree::Stm *func_body){
  // if(frame->GetLabel()=="main"){
  //   return {func_body};
  // }

  std::list<tree::Stm*> stm_list;
  auto callee_saved_regs = reg_manager->CalleeSaves()->GetList();
  std::vector<temp::Temp*> saved;//move remove and spill in regalloc
  // std::list<Access*> saved;//just simplify is ok

  auto framePtr = new tree::TempExp(reg_manager->FramePointer());
  //save registers
  for(const auto&reg:callee_saved_regs){
    auto temp = temp::TempFactory::NewTemp();
    // auto access = frame->AllocLocal(true);//not escape,but must put in frame
    saved.push_back(temp);
    // saved.push_back(access);
    stm_list.push_back(
      new tree::MoveStm(
        new tree::TempExp(temp),
        // access->ToExp(framePtr),
        new tree::TempExp(reg)
      )
    );
  }

  //view shift, place params
  auto view_shift = frame->GetVSList();
  stm_list.splice(stm_list.end(),view_shift);

  //func body codes
  stm_list.push_back(func_body);


  //restore registers
  auto saved_reg_it = saved.begin();
  for(const auto&reg:callee_saved_regs){
    stm_list.push_back(
      new tree::MoveStm(
        new tree::TempExp(reg),
        // (*saved_reg_it)->ToExp(framePtr)
        new tree::TempExp(*saved_reg_it)
      )
    );
    saved_reg_it++;
  }
  return stm_list;

}

Access *X64Frame::AllocLocal(bool escape){
  if(escape){
    sp_off -= reg_manager->WordSize();
    return new InFrameAccess(sp_off);
  }
  return new InRegAccess(temp::TempFactory::NewTemp());
}



assem::Proc *ProcEntryExit3(frame::Frame *frame,assem::InstrList *instr_list){
  std::stringstream prolog;
  auto frame_size = frame->GetFrameSize();
  //_scan_lines in interpreter.py
  prolog<<".set "<<frame->GetLabel()<<"_framesize, "<<frame_size<<std::endl;
  prolog<<frame->GetLabel()<<":"<<std::endl;
  // instr_list->Insert(instr_list->GetList().begin(),new assem::LabelInstr(frame->GetLabel(),frame->name_));
  prolog<<"subq $"<<frame_size<<",%rsp"<<std::endl;
  std::stringstream epilog;
  epilog<<"addq $"<<frame_size<<",%rsp"<<std::endl;
  epilog<<"retq"<<std::endl;
  return new assem::Proc(prolog.str(), instr_list,epilog.str());
}

assem::InstrList* ProcEntryExit2(assem::InstrList*body){
  body->Append(
    new assem::OperInstr(
      "", 
      new temp::TempList(),
      reg_manager->ReturnSink(),
      nullptr));
  return body;
}

} // namespace frame
