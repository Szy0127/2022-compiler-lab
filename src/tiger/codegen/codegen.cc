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
  fs_ = frame_->GetLabel();
  auto instr_list = new assem::InstrList();
  for (auto stm : traces_->GetStmList()->GetList()) {
    stm->Munch(*instr_list, fs_);
    // std::cout<<"stm begin"<<std::endl;
    // stm->Print(stderr,2);
    // std::cout<<"stm end"<<std::endl;
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
  left_->Munch(instr_list,fs);
  right_->Munch(instr_list,fs);
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  instr_list.Append(new assem::LabelInstr(temp::LabelFactory::LabelString(label_),label_));
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  instr_list.Append(
    new assem::OperInstr(
      "jmp `j0",
      new temp::TempList(), new temp::TempList(),
      new assem::Targets(jumps_)
    )
  );
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto left_temp =  left_->Munch(instr_list, fs);
  auto right_temp = right_->Munch(instr_list, fs);
  //TODO convert to double if one is int
  auto is_double = left_temp->IsDouble() || right_temp->IsDouble();
  instr_list.Append(
      new assem::OperInstr(
        //s1 - s0
        is_double ? "comisd `s0,`s1" :"cmpq `s0,`s1",
        new temp::TempList(),
        new temp::TempList{right_temp, left_temp},
        nullptr
      )
    );
  
  std::string op;
  switch (op_) {
    case EQ_OP:op = "je";break;
    case NE_OP:op = "jne";break;
    // left < right => left - right < 0
    case LT_OP:op = is_double ? "jb" : "jl";break;
    case GT_OP:op = is_double ? "ja" : "jg";break;
    case LE_OP:op = is_double ? " jbe" : "jle";break;
    case GE_OP:op = is_double ? "jae" : "jge";break;
  }
  std::stringstream assem;
  assem<<op<<" `j0";
  instr_list.Append(
    new assem::OperInstr(
      assem.str(),
      new temp::TempList(),
      new temp::TempList(),
      new assem::Targets(new std::vector<temp::Label *>{true_label_, false_label_})
    )
  );
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  if (typeid(*dst_) == typeid(MemExp)) {
    auto dst = static_cast<MemExp *>(dst_);
    if (typeid(*dst->exp_) == typeid(BinopExp)) {
      auto dst_mem = static_cast<BinopExp *>(dst->exp_);
      if (dst_mem->op_ == PLUS_OP) {
        std::stringstream assem;
        temp::Temp * src_temp = nullptr;
        int s_index = 0;
        if(typeid(*src_)==typeid(ConstExp)){
          assem<<"movq $"<<static_cast<ConstExp*>(src_)->consti_<<",";
        }else{
          src_temp = src_->Munch(instr_list,fs);
          assem<<"movq `s0,";
          s_index = 1;
        }


        //same as memexp
        // fp+const --> sp + const + framesize
        auto exp = dst_mem;
        if (typeid(*exp->left_) == typeid(ConstExp)) {
          auto left = static_cast<ConstExp *>(exp->left_);
          auto right_temp = exp->right_->Munch(instr_list, fs);
          if (right_temp == reg_manager->FramePointer()) {
            assem<<"("<<fs<<"_framesize";
            if(left->consti_>=0){
              assem<<"+";
            }
            assem<<left->consti_<<")(`s"<<s_index<<")";
            right_temp = reg_manager->StackPointer();
          } else {
            assem<<left->consti_<<"(`s"<<s_index<<")";
          }
          instr_list.Append(
            new assem::OperInstr(
              assem.str(),
              new temp::TempList(),
              src_temp ? new temp::TempList{src_temp,right_temp} : new temp::TempList(right_temp),
              nullptr
            )
          );
          return;
        }
        if (typeid(*exp->right_) == typeid(ConstExp)) {
          auto right = static_cast<ConstExp *>(exp->right_);
          auto left_temp = exp->left_->Munch(instr_list, fs);
          if (left_temp == reg_manager->FramePointer()) {
            assem<<"("<<fs<<"_framesize";
            if(right->consti_>=0){
              assem<<"+";
            }
            assem<<right->consti_<<")(`s"<<s_index<<")";
            left_temp = reg_manager->StackPointer();
          } else {
            assem<<right->consti_<<"(`s"<<s_index<<")";
          }
          instr_list.Append(
            new assem::OperInstr(
              assem.str(),
              new temp::TempList(),
              src_temp ? new temp::TempList{src_temp,left_temp}:new temp::TempList(left_temp),
              nullptr
            )
          );
          return;
        }


        auto left_temp = dst_mem->left_->Munch(instr_list, fs);
        auto right_temp = dst_mem->right_->Munch(instr_list, fs);
        assem<<"(`s"<<s_index<<",`s"<<s_index+1<<")";
        instr_list.Append(
          new assem::OperInstr(
            assem.str(),
            new temp::TempList(),
            src_temp ? new temp::TempList{src_temp,left_temp,right_temp}:new temp::TempList{left_temp,right_temp},
            nullptr
          )
        );
        return;
      }
    }
  
    auto src_temp = src_->Munch(instr_list, fs);
    auto dst_temp = dst->exp_->Munch(instr_list, fs);
    instr_list.Append(
      new assem::OperInstr(
        "movq `s0,(`s1)",
        new temp::TempList(),
        new temp::TempList{src_temp,dst_temp},
        nullptr
      )
    );
    return;
  }
  if (typeid(*src_) == typeid(MemExp)) {//restore callee saved registers cant use rax
    auto src = static_cast<MemExp *>(src_);
    if (typeid(*src->exp_) == typeid(BinopExp)) {
      auto src_mem = static_cast<BinopExp *>(src->exp_);
      if (src_mem->op_ == PLUS_OP) {
        auto dst_temp = dst_->Munch(instr_list,fs);
        std::stringstream assem;
        assem<<"movq ";

        //same as memexp
        // fp+const --> sp + const + framesize
        auto exp = src_mem;
        if (typeid(*exp->left_) == typeid(ConstExp)) {
          auto left = static_cast<ConstExp *>(exp->left_);
          auto right_temp = exp->right_->Munch(instr_list, fs);
          if (right_temp == reg_manager->FramePointer()) {
            assem<<"("<<fs<<"_framesize";
            if(left->consti_>=0){
              assem<<"+";
            }
            assem<<left->consti_<<")(`s0)";
            right_temp = reg_manager->StackPointer();
          } else {
            assem<<left->consti_<<"(`s0)";
          }
          assem<<",`d0";
          instr_list.Append(
            new assem::OperInstr(
              assem.str(),
              new temp::TempList(dst_temp),
              new temp::TempList{right_temp},
              nullptr
            )
          );
          return;
        }
        if (typeid(*exp->right_) == typeid(ConstExp)) {
          auto right = static_cast<ConstExp *>(exp->right_);
          auto left_temp = exp->left_->Munch(instr_list, fs);
          if (left_temp == reg_manager->FramePointer()) {
            assem<<"("<<fs<<"_framesize";
            if(right->consti_>=0){
              assem<<"+";
            }
            assem<<right->consti_<<")(`s0)";
            left_temp = reg_manager->StackPointer();
          } else {
            assem<<right->consti_<<"(`s0)";
          }
          assem<<",`d0";
          instr_list.Append(
            new assem::OperInstr(
              assem.str(),
              new temp::TempList(dst_temp),
              new temp::TempList{left_temp},
              nullptr
            )
          );
          return;
        }


        auto left_temp = src_mem->left_->Munch(instr_list, fs);
        auto right_temp = src_mem->right_->Munch(instr_list, fs);
        instr_list.Append(
          new assem::OperInstr(
            "movq (`s0,`s1),`d0",
            new temp::TempList(dst_temp),
            new temp::TempList{left_temp,right_temp},
            nullptr
          )
        );
        return;
      }
    }
  
    auto dst_temp = dst_->Munch(instr_list, fs);
    auto src_temp = src->exp_->Munch(instr_list, fs);
    instr_list.Append(
      new assem::OperInstr(
        "movq (`s0),`d0",
        new temp::TempList(dst_temp),
        new temp::TempList(src_temp),
        nullptr
      )
    );
    return;
  }

  if(typeid(*src_)==typeid(ConstExp)){
    auto src = static_cast<ConstExp *>(src_);
    auto dst = dst_->Munch(instr_list, fs);
    std::stringstream assem;
    assem << "movq $" << src->consti_ << ",`d0";
    instr_list.Append(
      new assem::OperInstr(
        assem.str(),
        new temp::TempList(dst),
        new temp::TempList(),//can be nullptr in Format,but cant in Print
        nullptr
      )
    );
    return;
  }


  //fp -> sp
  if(typeid(*src_)==typeid(BinopExp)){
    auto dst_temp = dst_->Munch(instr_list,fs);
    auto src = static_cast<BinopExp*>(src_);
    if(src->op_==PLUS_OP){
      std::stringstream assem;
      assem<<"leaq ";
      if (typeid(*src->left_) == typeid(ConstExp)) {
          auto left = static_cast<ConstExp *>(src->left_);
          auto right_temp = src->right_->Munch(instr_list, fs);
          if (right_temp == reg_manager->FramePointer()) {
            assem<<"("<<fs<<"_framesize";
            if(left->consti_>=0){
              assem<<"+";
            }
            assem<<left->consti_<<")(`s0)";
            right_temp = reg_manager->StackPointer();
          } else {
            assem<<left->consti_<<"(`s0)";
          }
          assem<<",`d0";
          instr_list.Append(
            new assem::OperInstr(
              assem.str(),
              new temp::TempList(dst_temp),
              new temp::TempList{right_temp},
              nullptr
            )
          );
          return;
        }
      if (typeid(*src->right_) == typeid(ConstExp)) {
          auto right = static_cast<ConstExp *>(src->right_);
          auto left_temp = src->left_->Munch(instr_list, fs);
          if (left_temp == reg_manager->FramePointer()) {
            assem<<"("<<fs<<"_framesize";
            if(right->consti_>=0){
              assem<<"+";
            }
            assem<<right->consti_<<")(`s0)";
            left_temp = reg_manager->StackPointer();
          } else {
            assem<<right->consti_<<"(`s0)";
          }
          assem<<",`d0";
          instr_list.Append(
            new assem::OperInstr(
              assem.str(),
              new temp::TempList(dst_temp),
              new temp::TempList{left_temp},
              nullptr
            )
          );
          return;
        }

      assem<<"(`s0,`s1),`d0";
      auto left_temp = src->left_->Munch(instr_list,fs);
      auto right_temp = src->right_->Munch(instr_list,fs);
      instr_list.Append(
        new assem::OperInstr(
          assem.str(),
          new temp::TempList(dst_temp),
          new temp::TempList{left_temp,right_temp},
          nullptr
        )
      );
      return;
    }

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
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  exp_->Munch(instr_list,fs);

}

//consider double,we need other instructions,so it will be much complicated if considering maximum munch

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */

  auto left_temp = left_->Munch(instr_list,fs);
  auto right_temp = right_->Munch(instr_list,fs);
  temp::Temp* result_temp;
  auto is_double = right_temp->IsDouble() || left_temp->IsDouble();
  if(is_double){
    result_temp = temp::TempFactory::NewTemp(false,true);
    instr_list.Append(
      new assem::MoveInstr(
        "movapd `s0,`d0",
        new temp::TempList(result_temp),
        new temp::TempList(left_temp)
      )
    );
  }else{
    result_temp = temp::TempFactory::NewTemp();
    instr_list.Append(
      new assem::MoveInstr(
        "movq `s0,`d0",
        new temp::TempList(result_temp),
        new temp::TempList(left_temp)
      )
    );
  }
  switch (op_)
  { 
  case PLUS_OP:{
    if(is_double){
      instr_list.Append(
      new assem::OperInstr(
          "addsd `s0,`d0",
          new temp::TempList(result_temp),
          new temp::TempList{right_temp,result_temp},
          nullptr
        )
      );
      return result_temp;
    }
    instr_list.Append(
    new assem::OperInstr(
        "addq `s0,`d0",
        new temp::TempList(result_temp),
        new temp::TempList{right_temp,result_temp},
        nullptr
      )
    );
    return result_temp;
    break;
    }
  case MINUS_OP:{
    if(is_double){
      instr_list.Append(
      new assem::OperInstr(
          "subsd `s0,`d0",
          new temp::TempList(result_temp),
          new temp::TempList{right_temp,result_temp},
          nullptr
        )
      );
      return result_temp;
    }
    instr_list.Append(
      new assem::OperInstr(
        "subq `s0,`d0",
        new temp::TempList(result_temp),
        new temp::TempList{right_->Munch(instr_list,fs),result_temp},
        nullptr
      )
    );
    return result_temp;
    break;

  }
  case MUL_OP:{
    if(is_double){
      instr_list.Append(
      new assem::OperInstr(
          "mulsd `s0,`d0",
          new temp::TempList(result_temp),
          new temp::TempList{right_temp,result_temp},
          nullptr
        )
      );
      return result_temp;
    }
    //%rdx %rax <-- S x %rax
      auto rax = reg_manager->GetRegister(0);
      auto rdx = reg_manager->GetRegister(3);
      instr_list.Append(
        new assem::MoveInstr(
          "movq `s0,%rax",//`d0?
          new temp::TempList(rax),
          new temp::TempList(left_temp)
        )
      );
      instr_list.Append(
        new assem::OperInstr(
          "imulq `s0",
          new temp::TempList{rax, rdx},
          new temp::TempList{right_temp, rax},
          nullptr
        )
      );
      return rax;
      break;
  }
  case DIV_OP:{
    if(is_double){
      instr_list.Append(
      new assem::OperInstr(
          "divsd `s0,`d0",
          new temp::TempList(result_temp),
          new temp::TempList{right_temp,result_temp},
          nullptr
        )
      );
      return result_temp;
    }
    //%rax <-- %rdx %rax / S  %rdx = mod
    auto rax = reg_manager->GetRegister(0);
    auto rdx = reg_manager->GetRegister(3);
    instr_list.Append(
      new assem::MoveInstr(
        "movq `s0,%rax",//`d0?
        new temp::TempList(rax),
        new temp::TempList(left_temp)
      )
    );
    instr_list.Append(
      new assem::OperInstr(
        "cqto",
        new temp::TempList(rdx),
        new temp::TempList(),
        nullptr
      )
    );
    instr_list.Append(
      new assem::OperInstr(
        "idivq `s0",
        new temp::TempList{rax, rdx},
        new temp::TempList{right_->Munch(instr_list,fs), rax,rdx},
        nullptr
      )
    );
    return rax;
    break;
  }
  case MOD_OP:{
      //if double,error

      //%rax <-- %rdx %rax / S  %rdx = mod
      auto rax = reg_manager->GetRegister(0);
      auto rdx = reg_manager->GetRegister(3);
      instr_list.Append(
        new assem::MoveInstr(
          "movq `s0,%rax",//`d0?
          new temp::TempList(rax),
          new temp::TempList(left_temp)
        )
      );
      instr_list.Append(
        new assem::OperInstr(
          "cqto",
          new temp::TempList(rdx),
          new temp::TempList(),
          nullptr
        )
      );
      instr_list.Append(
        new assem::OperInstr(
          "idivq `s0",
          new temp::TempList{rax, rdx},
          new temp::TempList{right_->Munch(instr_list,fs), rax,rdx},
          nullptr
        )
      );
      return rdx;
      break;
  }
  default:
    break;
  }

}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto ret_temp = temp::TempFactory::NewTemp();
  if (typeid(*exp_) == typeid(BinopExp)) {
    auto exp = static_cast<BinopExp *>(exp_);
    if (exp->op_ == PLUS_OP) {
      // fp+const --> sp + const + framesize
      if (typeid(*exp->left_) == typeid(ConstExp)) {
        auto left = static_cast<ConstExp *>(exp->left_);
        auto right_temp = exp->right_->Munch(instr_list, fs);
        std::stringstream assem;
        if (right_temp == reg_manager->FramePointer()) {
          assem<<"movq ("<<fs<<"_framesize";
            if(left->consti_>=0){
              assem<<"+";
            }
            assem<<left->consti_<<")(`s0),`d0";
          right_temp = reg_manager->StackPointer();
        } else {
          assem<<"movq "<<left->consti_<<"(`s0),`d0";
        }
        instr_list.Append(
          new assem::OperInstr(
            assem.str(),
            new temp::TempList(ret_temp),
            new temp::TempList(right_temp),
            nullptr
          )
        );
        return ret_temp;
      }
      if (typeid(*exp->right_) == typeid(ConstExp)) {
        auto right = static_cast<ConstExp *>(exp->right_);
        auto left_temp = exp->left_->Munch(instr_list, fs);
        std::stringstream assem;
        if (left_temp == reg_manager->FramePointer()) {
          assem<<"movq ("<<fs<<"_framesize";
          if(right->consti_>=0){
            assem<<"+";
          }
          assem<<right->consti_<< ")(`s0),`d0";
          left_temp = reg_manager->StackPointer();
        } else {
          assem<<"movq "<<right->consti_<<"(`s0),`d0";
        }
        instr_list.Append(
          new assem::OperInstr(
            assem.str(),
            new temp::TempList(ret_temp),
            new temp::TempList(left_temp),
            nullptr
          )
        );
        return ret_temp;
      }

      auto left_temp = exp->left_->Munch(instr_list,fs);
      auto right_temp = exp->right_->Munch(instr_list,fs);
      instr_list.Append(
        new assem::OperInstr(
          "movq (`s0,`s1),`d0",
          new temp::TempList(ret_temp),
          new temp::TempList{left_temp,right_temp},
          nullptr
        )
      );
    }
    return ret_temp;
  }
  instr_list.Append(
    new assem::OperInstr(
      "movq (`s0),`d0",
      new temp::TempList(ret_temp),
      new temp::TempList{exp_->Munch(instr_list,fs)},
      nullptr
    )
  );
  return ret_temp;
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

  /*
  stable_table.py
  if mem.startswith('L'):
      # Relative mem_address
      return self._string_address + int(mem[1:imm_index]) * 8
  
  */
  temp::Temp *str_addr_temp;
  std::stringstream assem;
  if(is_double_){
    assem << "movsd " << temp::LabelFactory::LabelString(name_) << "(%rip),`d0";
    str_addr_temp = temp::TempFactory::NewTemp(false,true);
  }else{
    assem << "leaq " << temp::LabelFactory::LabelString(name_) << "(%rip),`d0";
    str_addr_temp = temp::TempFactory::NewTemp(false,false);
  }
  instr_list.Append(
    new assem::OperInstr(
      assem.str(),
      new temp::TempList(str_addr_temp),
      new temp::TempList(),
      nullptr
    )
  );
  return str_addr_temp;
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

  // here to extend stack?

  auto rsp = reg_manager->StackPointer();
  // auto args_size = args_->GetList().size();
  // auto max_args_size = reg_manager->ArgRegs()->GetList().size();
  // int extend_stack = reg_manager->WordSize();// pointer_map_label
  // if(args_size > max_args_size){
  //   extend_stack +=  (args_size - max_args_size) * reg_manager->WordSize();
  // }

  auto not_main_level = pointer_map_->str_.empty();
  pointer_map_->str_ = std::to_string(arg_in_stack_ * (not_main_level ? 1:-1));
  std::cout<<arg_in_stack_<<std::endl;
  instr_list.Append(
    new assem::OperInstr(
      "subq $" + std::to_string( arg_in_stack_* reg_manager->WordSize()) + ",`d0",
      new temp::TempList(rsp),
      new temp::TempList(rsp),
      nullptr
    )
  );
  
  auto pointer_map_temp = temp::TempFactory::NewTemp();
  instr_list.Append(
    new assem::OperInstr(
      "leaq " + temp::LabelFactory::LabelString(pointer_map_->label_) + "(%rip),`d0",
      new temp::TempList(pointer_map_temp),
      new temp::TempList(),
      nullptr
    )
  );
  instr_list.Append(
    new assem::OperInstr(
      "movq `s0,(`s1)",
      new temp::TempList(),
      new temp::TempList{pointer_map_temp,reg_manager->StackPointer()},
      nullptr
    )
  );


  auto calldefs = reg_manager->CallerSaves();

  calldefs->Append(reg_manager->ReturnValue());
  instr_list.Append(
    new assem::OperInstr(
      "callq " + temp::LabelFactory::LabelString(static_cast<NameExp *>(fun_)->name_),
      calldefs, args,
      nullptr,pointer_map_,arg_in_stack_
    )
  );


  instr_list.Append(
    new assem::OperInstr(
      "addq $" +std::to_string(arg_in_stack_* reg_manager->WordSize())+",`d0",
      new temp::TempList(rsp),
      new temp::TempList(rsp),
      nullptr
    )
  );
  

  return reg_manager->ReturnValue();
}

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto arg_regs = reg_manager->ArgRegs()->GetList();
  auto double_regs = reg_manager->DoubleRegs()->GetList();
  auto i = 0;

  //TODO int:6 double:8
  auto max_index = arg_regs.size();
  auto arg_it = arg_regs.begin();
  auto arg_double_it = double_regs.begin();
  // match with frame::frame
  auto used_temps = new temp::TempList();
  auto wordsize = reg_manager->WordSize();

  auto arg_size = exp_list_.size();
  for(const auto&exp:exp_list_){
    auto temp = exp->Munch(instr_list,fs);
    if(i < max_index){
      if(i==0 && temp == reg_manager->FramePointer()){//static link may use rbp
        std::stringstream assem;
        assem << "leaq " << fs << "_framesize(`s0),`d0";
        instr_list.Append(
          new assem::OperInstr(
            assem.str(),
            new temp::TempList(*arg_it),
            new temp::TempList(reg_manager->StackPointer()),
            nullptr
          )
        );
        used_temps->Append(*arg_it);
        arg_it++;
        //double?
      }else{
        if(temp->IsDouble()){
            instr_list.Append(
            new assem::MoveInstr(
              "movapd `s0, `d0",
              new temp::TempList(*arg_double_it),
              new temp::TempList(temp)
            )
          );
          // std::cout<<(*arg_double_it)->Int();
          used_temps->Append(*arg_double_it);
          arg_double_it++;
        }else{
          instr_list.Append(
            new assem::MoveInstr(
              "movq `s0,`d0",
              new temp::TempList(*arg_it),
              new temp::TempList(temp)
            )
          );
          used_temps->Append(*arg_it);
          arg_it++;
        }
      }
    }else{
      std::stringstream assem;
      //rsp-->
      //rsp-8   param -1th
      //rsp-16 param -2th
      //rsp-8*n param 7th
      assem << "movq `s0," << (int)(-wordsize * (arg_size-i)) << "(`s1)";
      instr_list.Append(
        new assem::OperInstr(
          assem.str(),
          new temp::TempList(),
          new temp::TempList{temp,reg_manager->StackPointer()},
          nullptr
        )
      );
    } 
    i++;
  }
  if(i > max_index){
    used_temps->Append(reg_manager->StackPointer());
  }
  return used_temps;
}

} // namespace tree
