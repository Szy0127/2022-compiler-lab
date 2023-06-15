#include "tiger/absyn/absyn.h"
#include "tiger/semant/semant.h"
#include <set>
// #include <iostream>
namespace absyn {

void AbsynTree::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                           err::ErrorMsg *errormsg) const {
   
  root_->SemAnalyze(venv,tenv,0,errormsg);
}

type::Ty *SimpleVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
   
  auto entry = venv->Look(sym_);
  if (entry && typeid(*entry) == typeid(env::VarEntry)) {
    if(static_cast<env::VarEntry *>(entry)->ty_){
      return (static_cast<env::VarEntry *>(entry))->ty_->ActualTy();
    }else{
      return nullptr;
    }
  } else {
    return nullptr;
    // errormsg->Error(pos_, "undefined variable %s", sym_->Name().data());
  }
  return type::IntTy::Instance();
}

type::Ty *FieldVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
   
  auto ty = var_->SemAnalyze(venv,tenv,labelcount,errormsg)->ActualTy();
  if(typeid(*ty)!=typeid(type::RecordTy)){
    errormsg->Error(var_->pos_,"not a record type");
    return type::IntTy::Instance();
  }
  for(const auto &field:static_cast<type::RecordTy*>(ty)->fields_->GetList()){
    if(field->name_==sym_){
      return field->ty_;
    }
  }
  errormsg->Error(var_->pos_,"field %s doesn't exist", sym_->Name().data());
  return type::IntTy::Instance();
}

type::Ty *SubscriptVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount,
                                   err::ErrorMsg *errormsg) const {
   
  auto ty = var_->SemAnalyze(venv,tenv,labelcount,errormsg)->ActualTy();
  if(typeid(*ty)!=typeid(type::ListTy)){
    errormsg->Error(var_->pos_,"list type required");
    return type::IntTy::Instance();
  }
  auto exp_ty = subscript_->SemAnalyze(venv,tenv,labelcount,errormsg)->ActualTy();
  if(typeid(*exp_ty)!=typeid(type::IntTy)){
    errormsg->Error(var_->pos_,"not int type in SubscriptVar");
    return type::IntTy::Instance();
  }
  //exam length
  // return static_cast<type::ArrayTy*>(ty)->ty_->ActualTy();
  return type::IntTy::Instance();
}

type::Ty *VarExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
   
  return var_->SemAnalyze(venv,tenv,labelcount,errormsg);
}

type::Ty *NilExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
   
  return type::NilTy::Instance();
}

type::Ty *IntExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
   
  return type::IntTy::Instance();
}
type::Ty *DoubleExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
   
  return type::DoubleTy::Instance();
}
type::Ty *StringExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
   
  return type::StringTy::Instance();
}

type::Ty *CallExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
   
  auto fun_entry = venv->Look(func_);
  if(!fun_entry){
    errormsg->Error(pos_, "undefined function %s", func_->Name().data());
    return type::VoidTy::Instance();
  }
  auto formals = static_cast<env::FunEntry*>(fun_entry)->formals_;
  auto result = static_cast<env::FunEntry*>(fun_entry)->result_;

  auto formal_it = formals->GetList().begin();
  auto param_it = args_->GetList().begin();

  auto formal_end = formals->GetList().end();
  auto param_end = args_->GetList().end();
  // for(;param_it != param_end && formal_it != formal_end;param_it++,formal_it++){
  //   auto param_ty = (*param_it)->SemAnalyze(venv,tenv,labelcount,errormsg);
  //   if(!param_ty->IsSameType(*formal_it)){
  //     errormsg->Error((*param_it)->pos_,"para type mismatch");
  //   }
  // }
  // if(param_it != param_end){
  //   errormsg->Error(pos_,"too many params in function %s",func_->Name().data());
  // }
  // if(formal_it != formal_end){
  //   errormsg->Error(pos_,"too few params in function %s",func_->Name().data());
  // }


  return result;
}

type::Ty *OpExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
   
  type::Ty *left_ty = nullptr;
  if(left_){
    left_ty = left_->SemAnalyze(venv,tenv,labelcount,errormsg);//->ActualTy();
  }
  auto right_ty = right_->SemAnalyze(venv,tenv,labelcount,errormsg);//->ActualTy();

  if (oper_ == absyn::PLUS_OP || oper_ == absyn::MINUS_OP || 
      oper_ == absyn::TIMES_OP || oper_ == absyn::DIVIDE_OP || oper_ == absyn::MOD_OP) {
    // if (typeid(*left_ty) != typeid(type::IntTy)) {
    //   errormsg->Error(left_->pos_,"integer required");
    // }
    // if (typeid(*right_ty) != typeid(type::IntTy)) {
    //   errormsg->Error(right_->pos_,"integer required");
    // }
    return type::IntTy::Instance();
  }
  // if (!left_ty->IsSameType(right_ty)) {
  //   errormsg->Error(pos_, "same type required");
  // }
  return type::IntTy::Instance();
}

type::Ty *RecordExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
   
  auto ty = tenv->Look(typ_);
  if(!ty){
    errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
    return type::VoidTy::Instance();
  }
  if(typeid(*ty)!=typeid(type::RecordTy)){
    errormsg->Error(pos_, "not a record type");
    return type::VoidTy::Instance();
  }
  auto field_list = static_cast<type::RecordTy*>(ty)->fields_->GetList();
  for(const auto &efield:fields_->GetList()){
    auto exp_ty = efield->exp_->SemAnalyze(venv,tenv,labelcount,errormsg)->ActualTy();
    // auto found = false;
    // for(const auto field:field_list){
    //   if(efield->name_ == field->name_){
    //     found = true;
    //     if(!exp_ty->IsSameType(field->ty_)){
    //       errormsg->Error(efield->exp_->pos_, "record type not match");
    //     }
    //     break;
    //   }
    // }
    // auto it = std::find_if(field_list.begin(),field_list.end(),[&efield](const auto &field){return efield->name_==field->name_;});
    // assert((it!=field_list.end())==found);
    if(auto it = std::find_if(field_list.begin(),field_list.end(),
        [&efield,&exp_ty](const auto &field){
          return efield->name_==field->name_ && exp_ty->IsSameType(field->ty_);
          });
        it==field_list.end()){
       errormsg->Error(pos_, "record type not found");
    }
  }
  return ty;
}

type::Ty *SeqExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
   
  type::Ty *result;
  for(const auto &exp: seq_->GetList()){
    result = exp->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  return result;
}

type::Ty *AssignExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  auto var_ty = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  auto exp_ty = exp_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  //simple var not defined
  if(!var_ty){
    if(typeid(*var_)!=typeid(SimpleVar)){
      errormsg->Error(var_->pos_, "undefined variable");
      return type::VoidTy::Instance();
    }
    auto sym = static_cast<SimpleVar*>(var_)->sym_;
    venv->Enter(sym,new env::VarEntry(exp_ty));
    return type::VoidTy::Instance();
  }
  var_ty = var_ty->ActualTy();
  if(!var_ty->IsSameType(exp_ty)){
    errormsg->Error(var_->pos_,"unmatched assign exp");
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
   
  auto test_ty = test_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  auto then_ty = then_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  type::Ty *else_ty = type::VoidTy::Instance();
  if(elsee_){
    else_ty = elsee_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  }
  //dont know what type func returns
  // if(typeid(*test_ty) != typeid(type::IntTy)) {
  //   errormsg->Error(test_->pos_, "if exp's range type is not integer");
  // }
  // if(!then_ty->IsSameType(else_ty)){
  //   if(!elsee_){
  //     errormsg->Error(pos_, "if-then exp's body must produce no value");
  //   }else{
  //     errormsg->Error(then_->pos_, "then exp and else exp type mismatch");
  //   }
   
  // }
  return then_ty;
}

type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
   
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
   
  if(labelcount==0){
    errormsg->Error(pos_,"break is not inside any loop");
  }
  return type::VoidTy::Instance();
}

type::Ty *ReturnExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
   
  if(labelcount==0){
    errormsg->Error(pos_,"return is not inside any function");
  }
  ret_->SemAnalyze(venv, tenv, labelcount-1, errormsg);
  return type::VoidTy::Instance();
}

type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
   
  venv->BeginScope();
  tenv->BeginScope();
  auto decslist = decs_->GetList();
  for (const auto &dec : decslist){
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

type::Ty *FunctionExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  funcs_->SemAnalyze(venv, tenv, labelcount, errormsg);
  return type::VoidTy::Instance();
}


type::Ty *ArrayExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
   
  auto ty = tenv->Look(typ_);
  if(!ty){
    errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
  }
  if(typeid(*ty)!=typeid(type::ArrayTy)){
    errormsg->Error(pos_, "array type required");
    return type::VoidTy::Instance();
  }
  auto size_ty = size_->SemAnalyze(venv,tenv,labelcount,errormsg)->ActualTy();
  auto init_ty = init_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(typeid(*size_ty)!=typeid(type::IntTy)){
    errormsg->Error(pos_, "size should be int");
  }
  if(!init_ty->IsSameType(static_cast<type::ArrayTy*>(ty)->ty_)){
    errormsg->Error(pos_, "array type mismatch");
  }
  return ty;
}

type::Ty *ListExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
   
  return type::ListTy::Instance();
}

type::Ty *VoidExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
   
  return type::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
   
  auto func_list = functions_->GetList();

  for(const auto&function:func_list){
    auto params = function->params_;
    type::Ty *result_ty = type::VoidTy::Instance();
    if(function->result_){
      result_ty = tenv->Look(function->result_); 
    }
    auto formals = params->MakeFormalTyList(tenv, errormsg);
    if(venv->Look(function->name_)){
      errormsg->Error(pos_, "two functions have the same name");
    }
    venv->Enter(function->name_, new env::FunEntry(formals, result_ty));
  }
  for(const auto&function:func_list){
    auto params = function->params_;
    auto formals = params->MakeFormalTyList(tenv, errormsg);
    venv->BeginScope();
    auto formal_it = formals->GetList().begin();
    auto param_it = params->GetList().begin();
    for (; param_it != params->GetList().end(); formal_it++, param_it++)
        venv->Enter((*param_it)->name_, new env::VarEntry(*formal_it));
    auto res = function->body_->SemAnalyze(venv, tenv, labelcount+1, errormsg)->ActualTy();

    type::Ty *result_ty = type::VoidTy::Instance();

    if(function->result_){
      result_ty = tenv->Look(function->result_); 
    }


    // if(typeid(*result_ty->ActualTy())==typeid(type::VoidTy)
    //   && typeid(*res)!=typeid(type::VoidTy)){
    //   errormsg->Error(pos_, "procedure returns value");
    // }else{
    //   if(!result_ty->IsSameType(res)){
    //     errormsg->Error(pos_, "function return value mismatch");
    //   }
    // }
    venv->EndScope();
  }

}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
   
  auto init_ty = init_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  if(typ_){
    auto ty = tenv->Look(typ_);
    if(!ty->IsSameType(init_ty)){
      errormsg->Error(pos_, "type mismatch");
    }
  }else{
    if(typeid(*init_ty)==typeid(type::NilTy)){
      errormsg->Error(pos_, "init should not be nil without type specified");
    }
  }
  venv->Enter(var_, new env::VarEntry(init_ty));
}

void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                         err::ErrorMsg *errormsg) const {
   
  auto type_list = types_->GetList();
  for(const auto &type:type_list){
    if(tenv->Look(type->name_)){
      errormsg->Error(pos_, "two types have the same name");
    }
    tenv->Enter(type->name_, new type::NameTy(type->name_,nullptr)); 
  }
  for(const auto &type:type_list){
    
    auto ty = type->ty_->SemAnalyze(tenv, errormsg);
    tenv->Enter(type->name_, ty); 
    if(typeid(*ty)==typeid(type::RecordTy)){
      auto fields_list = static_cast<type::RecordTy*>(ty)->fields_->GetList();
      for(auto&field:fields_list){
        if(!field->ty_){
          continue;
        }
        if(typeid(*field->ty_)==typeid(type::NameTy)){
          auto ty_ = static_cast<type::NameTy*>(field->ty_);
          if(!ty_->ty_){
            if(ty_->sym_ == type->name_){//type list = { first: int, rest: list }
              field->ty_ = ty;
            }else{
              errormsg->Error(pos_, "undefined type %s", ty_->sym_->Name().data());
            }
          }
        }
      }
    }
  }

  auto len = type_list.size();
  //cycle for name type
  for(const auto &type:type_list){
    auto sym = type->name_;
    for(auto i = 0 ;i < len;i++){
      auto ty = tenv->Look(sym);
      if(typeid(*ty)!=typeid(type::NameTy)){
        return;
      }
      sym = static_cast<type::NameTy*>(ty)->sym_;
      if(type->name_ == sym){
        errormsg->Error(pos_, "illegal type cycle");
        return;
      }
    }

  }

}

type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
   
  auto ty = tenv->Look(name_);
  if(ty) {
    return new type::NameTy(name_,ty);
  }
  errormsg->Error(pos_, "undefined type %s", name_->Name().data());
  return type::VoidTy::Instance();
}

type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const {
   

  return new type::RecordTy(record_->MakeFieldList(tenv,errormsg));
}

type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
   
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

} // namespace tr
