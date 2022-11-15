#include "tiger/codegen/codegen.h"

#include <cassert>
#include <sstream>
#include <iostream>

extern frame::RegManager *reg_manager;

namespace {

constexpr int maxlen = 1024;


} // namespace

namespace cg {

void CodeGen::Codegen() {
  /* TODO: Put your lab5 code here */

  //cant assem::InstrList instr_list and make_unique &instr_list; 
  auto instr_list = new assem::InstrList();
  for (auto stm : traces_->GetStmList()->GetList()) {
    stm->Munch(*instr_list, fs_);
  }
  assem_instr_ = std::make_unique<AssemInstr>(frame::ProcEntryExit2(instr_list));
}

void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}
} // namespace cg

namespace tree {
/* TODO: Put your lab5 code here */

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  std::cout<<"seq"<<std::endl;
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  instr_list.Append(new assem::LabelInstr(temp::LabelFactory::LabelString(label_),label_));
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  std::cout<<"JumpStm"<<std::endl;
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  std::cout<<"CjumpStm"<<std::endl;
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  if(typeid(*src_)==typeid(ConstExp)){
    auto src = static_cast<ConstExp *>(src_);
    auto dst = dst_->Munch(instr_list, fs);
    std::stringstream assem;
    assem << "movq $" << src->consti_ << ",`d0";
    instr_list.Append(
      new assem::MoveInstr(
        assem.str(),
        new temp::TempList(dst),
        new temp::TempList()//can be nullptr in Format,but cant in Print
      )
    );
  }
  auto src = src_->Munch(instr_list,fs);
  auto dst = dst_->Munch(instr_list,fs);
  instr_list.Append(
    new assem::MoveInstr(
      "movq `s0,`d0",
      new temp::TempList(dst),
      new temp::TempList(src)
    )
  );
  // if (typeid(*dst_) == typeid(MemExp)) {
  //   auto dst_mem = static_cast<MemExp*>(dst_);
  //   if (typeid(*dst_mem->exp_) == typeid(BinopExp)) {
  //     auto dst_binop = static_cast<BinopExp>(dst_mem->exp_);
  //     if(dst_binop == PLUS_OP && 
  //       typeid(*dst_binop->right_) == typeid(ConstExp)) {
  //       auto e1 = dst_binop->left_; 
  //       auto e2 = src_;
  //       /*MOVE(MEM(e1+i), e2) */
  //       e1->Munch(instr_list,fs);
  //       e2->Munch(instr_list,fs);
  //       // il.emit(“STORE”);
  //       return;
  //     }
  //     if(dst_binop->op_== PLUS_OP &&
  //       typeid(*dst_binop->left_) == typeid(ConstExp)) {
  //       auto e1 = dst_binop->right_; 
  //       auto e2 = src_;
  //       /*MOVE(MEM(i+e1), e2) */
  //       e1->Munch(instr_list,fs);
  //       e2->Munch(instr_list,fs);	
  //       // il.emit(“STORE”);
  //       return;
  //     }

  //   }

  
  // if (typeid(*dst_) == typeid(TempExp)) {
  //   auto e2=src_;
  //   /*MOVE(TEMP~i, e2) */
  //   auto temp = e2->Munch(instr_list,fs); 


}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  exp_->Munch(instr_list,fs);

}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto left_temp = left_->Munch(instr_list,fs);
  auto right_temp = right_->Munch(instr_list,fs);
  auto result_temp = temp::TempFactory::NewTemp();
  instr_list.Append(
    new assem::MoveInstr(
      "movq `s0,`d0",
      new temp::TempList(result_temp),
      new temp::TempList(left_temp)
    )
  );
  switch (op_)
  { 
  case PLUS_OP:
    instr_list.Append(
      new assem::OperInstr(
        "addq `s0,`d0",
        new temp::TempList(result_temp),
        new temp::TempList(right_temp),
        nullptr
      )
    );
    break;
  case MINUS_OP:
    instr_list.Append(
       new assem::OperInstr(
        "subq `s0,`d0",
        new temp::TempList(result_temp),
        new temp::TempList(right_temp),
        nullptr
       )
    );
    break;
  
  default:
    break;
  }
  return result_temp;

}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  std::cout<<"MemExp"<<std::endl;
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  return temp_;
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  stm_->Munch(instr_list,fs);
  return exp_->Munch(instr_list,fs);
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  std::cout<<"NameExp"<<std::endl;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto const_temp = temp::TempFactory::NewTemp();
  std::stringstream assem;
  assem << "movq $" << consti_ << ",`d0";
  instr_list.Append(
    new assem::OperInstr(
      assem.str(),
      new temp::TempList(const_temp),
      new temp::TempList(),
      nullptr
    )
  );
  return const_temp;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */

  // temp::Temp *func_name = fun_->Munch(instr_list) ;
  auto args = args_->MunchArgs(instr_list,fs);
  // args->Prepend(func_name);
  // x86-64 can directly call function name(label)
  // il.Append(new assem::OperInstr(“CALL `s0\n”, calldefs, args, nullptr));

  //caller saved registers


  auto calldefs = reg_manager->CallerSaves();
  calldefs->Append(reg_manager->ReturnValue());

  std::stringstream assem;
  assem << "callq " << temp::LabelFactory::LabelString(static_cast<NameExp *>(fun_)->name_);
  instr_list.Append(
    new assem::OperInstr(
      assem.str(),
      calldefs, args,
      nullptr
    )
  );
  return reg_manager->ReturnValue();
}

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto arg_regs = reg_manager->ArgRegs()->GetList();
  auto i = 0;
  auto max_index = arg_regs.size();
  auto arg_it = arg_regs.begin();
  // match with frame::frame
  auto used_temps = new temp::TempList();
  for(const auto&exp:exp_list_){
    auto temp = exp->Munch(instr_list,fs);
    if(i < max_index){
      used_temps->Append(*arg_it);
      instr_list.Append(
        new assem::MoveInstr(
          "movq `s0,`d0",
          new temp::TempList(*arg_it),
          new temp::TempList(temp)
        )
      );
    }
    // else{
    //     instr_list.Append(
    //     new assem::MoveInstr(
    //       "movq `s0,`d0",
    //       new temp::TempList(*arg_it),
    //       new temp::TempList(temp)
    //     );
    //   )
    // }
    i++;
    arg_it++;
  }
  return used_temps;
}

} // namespace tree
