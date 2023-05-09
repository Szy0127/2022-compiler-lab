#include "tiger/escape/escape.h"
#include "tiger/absyn/absyn.h"

namespace esc {
void EscFinder::FindEscape() { absyn_tree_->Traverse(env_.get()); }
} // namespace esc

namespace absyn {

void AbsynTree::Traverse(esc::EscEnvPtr env) {
  /* TODO: Put your lab5 code here */
  root_->Traverse(env,0);
}

void SimpleVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  auto entry = env->Look(sym_);
  if(!entry){ // type checking return error
    return;
  }
  if(entry->depth_ < depth){
    *(entry->escape_) = true;
  }
}

void FieldVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  var_->Traverse(env,depth);
}

void SubscriptVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  var_->Traverse(env,depth);
}

void VarExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  var_->Traverse(env,depth);
}

void NilExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

void IntExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

void StringExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

void CallExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  auto exps = args_->GetList();
  for(auto &exp:exps){
    exp->Traverse(env,depth);
  }
}

void OpExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  if(left_)left_->Traverse(env,depth);
  right_->Traverse(env,depth);

}

void RecordExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  auto efields = fields_->GetList();
  for(auto &efield:efields){
    efield->exp_->Traverse(env,depth);
  }
}

void SeqExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  auto exps = seq_->GetList();
  for(auto &exp:exps){
    exp->Traverse(env,depth);
  }
}

void AssignExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  var_->Traverse(env,depth);
  exp_->Traverse(env,depth);
}

void IfExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  test_->Traverse(env,depth);
  then_->Traverse(env,depth);
  if(elsee_){
    elsee_->Traverse(env,depth);
  }
}

void WhileExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  test_->Traverse(env,depth);
  body_->Traverse(env,depth);
}

void ForExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  lo_->Traverse(env,depth);
  hi_->Traverse(env,depth);
  escape_ = false;
  env->Enter(var_,new esc::EscapeEntry(depth,&escape_));
  body_->Traverse(env,depth);
}

void BreakExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}
void ReturnExp::Traverse(esc::EscEnvPtr env, int depth) {
  ret_->Traverse(env,depth);
  return;
}

void LetExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  auto declist = decs_->GetList();
  for(auto &dec:declist){
    dec->Traverse(env,depth);
  }
  body_->Traverse(env,depth);
}
void FunctionExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  funcs_->Traverse(env,depth);
}

void ArrayExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  size_->Traverse(env,depth);
  init_->Traverse(env,depth);
}

void ListExp::Traverse(esc::EscEnvPtr env, int depth) {

}


void VoidExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

void FunctionDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  auto funclist = functions_->GetList();
  for(auto &funcdec:funclist){
    // funcdec->Traverse(env,depth+1);
    auto fieldlist = funcdec->params_->GetList();
    for(auto &field:fieldlist){
      field->escape_ = false;
      env->Enter(field->name_,new esc::EscapeEntry(depth+1,&(field->escape_)));
    }
    funcdec->body_->Traverse(env,depth+1);
  }
}

void VarDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  init_->Traverse(env,depth);
  escape_ = false;
  env->Enter(var_,new esc::EscapeEntry(depth,&escape_));
}

void TypeDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

} // namespace absyn
