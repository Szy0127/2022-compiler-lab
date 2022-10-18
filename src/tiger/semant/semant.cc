#include "tiger/absyn/absyn.h"
#include "tiger/semant/semant.h"

namespace absyn {

void AbsynTree::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  root_->SemAnalyze(venv,tenv,0,errormsg);
}

type::Ty *SimpleVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto entry = venv->Look(sym_);
  if (entry && typeid(*entry) == typeid(env::VarEntry)) {
    return (static_cast<env::VarEntry *>(entry))->ty_->ActualTy();
  } else {
    errormsg->Error(pos_, "undefined variable %s", sym_->Name().data());
  }
  return type::IntTy::Instance();
}

type::Ty *FieldVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
}

type::Ty *SubscriptVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
}

type::Ty *VarExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return var_->SemAnalyze(venv,tenv,labelcount,errormsg);
}

type::Ty *NilExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::NilTy::Instance();
}

type::Ty *IntExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::IntTy::Instance();
}

type::Ty *StringExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::StringTy::Instance();
}

type::Ty *CallExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
}

type::Ty *OpExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto left_ty = left_->SemAnalyze(venv,tenv,labelcount,errormsg)->ActualTy();
  auto right_ty = right_->SemAnalyze(venv,tenv,labelcount,errormsg)->ActualTy();

  if (oper_ == absyn::PLUS_OP || oper_ == absyn::MINUS_OP || 
      oper_ == absyn::TIMES_OP || oper_ == absyn::DIVIDE_OP) {
    if (typeid(*left_ty) != typeid(type::IntTy)) {
      errormsg->Error(left_->pos_,"integer required");
    }
    if (typeid(*right_ty) != typeid(type::IntTy)) {
      errormsg->Error(right_->pos_,"integer required");
    }
    return type::IntTy::Instance();
  }
  if (!left_ty->IsSameType(right_ty)) {
    errormsg->Error(pos_, "same type required");
  }
  return type::IntTy::Instance();
}

type::Ty *RecordExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */

  auto ty = tenv->Look(typ_);
  // for(auto efield:fields_->GetList()){

  // }
  if(!ty){
    errormsg->Error(pos_, "undefined variable %s", typ_->Name().data());
    return type::VoidTy::Instance();
  }
  return ty;
}

type::Ty *SeqExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *result;
  for(auto exp: seq_->GetList()){
    result = exp->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  return result;
}

type::Ty *AssignExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto var_ty = var_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  auto exp_ty = exp_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  if(!var_ty->IsSameType(exp_ty)){
    errormsg->Error(var_->pos_,"same type required");
  }
  if(typeid(*var_)==typeid(SimpleVar)){
    auto sym = static_cast<SimpleVar*>(var_)->sym_;
    auto entry = venv->Look(sym);
    if (!entry) {
      errormsg->Error(var_->pos_, "undefined variable %s", sym->Name().data());
    }else{
      if(entry->readonly_){
        errormsg->Error(var_->pos_,"loop variable can't be assigned");
      }else{
        venv->Enter(sym,new env::VarEntry(exp_ty));
      }
    }
  }
  return type::VoidTy::Instance();
}

type::Ty *IfExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto test_ty = test_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  auto then_ty = then_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  type::Ty *else_ty = type::VoidTy::Instance();
  if(elsee_){
    else_ty = test_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  }
  if(typeid(*test_ty) != typeid(type::IntTy)) {
    errormsg->Error(test_->pos_, "if exp's range type is not integer");
  }
  if(!then_ty->IsSameType(else_ty)) {
    errormsg->Error(then_->pos_, "same type required");
  }
  return then_ty;
}

type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto ty = test_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  if (typeid(*ty) != typeid(type::IntTy)) {
    errormsg->Error(test_->pos_,"while exp's range type is not integer");
  }
  if (!body_){
    return type::VoidTy::Instance();
  }

  ty = body_->SemAnalyze(venv, tenv, labelcount+1, errormsg)->ActualTy();
  if(typeid(*ty) != typeid(type::VoidTy)) {
    errormsg->Error(body_->pos_, "while body must produce no value");
  }
  return type::VoidTy::Instance();

}

type::Ty *ForExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto low_ty = lo_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  auto high_ty = hi_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();

  if (typeid(*low_ty) != typeid(type::IntTy)) {
    errormsg->Error(lo_->pos_,"for exp's range type is not integer");
  }
  if (typeid(*high_ty) != typeid(type::IntTy)) {
    errormsg->Error(hi_->pos_,"for exp's range type is not integer");
  }
  venv->BeginScope();
  venv->Enter(var_,new env::VarEntry(type::IntTy::Instance(),true));
  auto ty = body_->SemAnalyze(venv, tenv, labelcount+1, errormsg)->ActualTy();
  if (typeid(*ty) != typeid(type::VoidTy)) {
    errormsg->Error(hi_->pos_,"for body must produce no value");
  }
  venv->EndScope();
  return type::VoidTy::Instance();
}

type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
}

type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  venv->BeginScope();
  tenv->BeginScope();
  auto decslist = decs_->GetList();
  for (auto dec : decslist){
      dec->SemAnalyze(venv, tenv, labelcount, errormsg);
  }

  type::Ty *result;
  if (!body_){
    result = type::VoidTy::Instance();
  }else{ 
    result = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  tenv->EndScope();
  venv->EndScope();
  return result;
}

type::Ty *ArrayExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
}

type::Ty *VoidExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto function = functions_->GetList().front();
  auto params = function->params_;
  auto result_ty = tenv->Look(function->result_); 
  auto formals = params->MakeFormalTyList(tenv, errormsg);
  venv->Enter(function->name_, new env::FunEntry(formals, result_ty));
  venv->BeginScope();
  auto formal_it = formals->GetList().begin();
  auto param_it = params->GetList().begin();
  for (; param_it != params->GetList().end(); formal_it++, param_it++)
      venv->Enter((*param_it)->name_, new env::VarEntry(*formal_it));
  function->body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  venv->EndScope();
}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto init_ty = init_->SemAnalyze(venv, tenv, labelcount, errormsg);
  venv->Enter(var_, new env::VarEntry(init_ty));
}

void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                         err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  for(auto type:types_->GetList()){
    tenv->Enter(type->name_, type->ty_->SemAnalyze(tenv, errormsg)); 
  }
}

type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto ty = tenv->Look(name_);
  if(ty) {
    return new type::NameTy(name_,ty);
  }
  return new type::NameTy(sym::Symbol::UniqueSymbol("int"),type::IntTy::Instance());
}

type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */

  return new type::RecordTy(record_->MakeFieldList(tenv,errormsg));
}

type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto item_ty = tenv->Look(array_);
  if(item_ty) {
    return new type::ArrayTy(item_ty);
  }
  errormsg->Error(pos_, "undefined type %s", array_->Name().data());
  return new type::ArrayTy(type::IntTy::Instance());
}

} // namespace absyn

namespace sem {

void ProgSem::SemAnalyze() {
  FillBaseVEnv();
  FillBaseTEnv();
  absyn_tree_->SemAnalyze(venv_.get(), tenv_.get(), errormsg_.get());
}
} // namespace sem
