#include "tiger/codegen/codegen.h"

#include <cassert>
#include <sstream>

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
  assem_instr_ = std::make_unique<AssemInstr>(instr_list);
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
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  if(typeid(*src_)==typeid(ConstExp)){
    auto src = static_cast<ConstExp *>(src_);
    auto dst = dst_->Munch(instr_list, fs);
    std::stringstream assem;
    assem << "movq $" << src->consti_ << ",`d0";
    instr_list.Append(
      new assem::OperInstr(
        assem.str(),
        new temp::TempList(dst),
        nullptr,
        nullptr
      )
    );
  }
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
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  return temp_;
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  // return 
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

} // namespace tree
