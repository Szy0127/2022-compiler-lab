#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>

#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/x64frame.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/frame.h"
#include <sstream>
#include <map>
#include <set>
// #include <iostream>


#define NOP (new tr::ExExp(new tree::ConstExp(0)))
extern frame::Frags *frags;
extern frame::RegManager *reg_manager;
extern bool compile_function;
static std::map<std::string,std::set<uint64_t>> function2keys;

namespace tr {

Access *Access::AllocLocal(Level *level, bool escape,bool is_pointer) {
  /* TODO: Put your lab5 code here */
  return new Access(level,level->frame_->AllocLocal(escape,is_pointer));
}

frame::StringFrag* GetPointerMap(frame::Frame* frame,tr::Level *level){
  auto pointer_map_label = temp::LabelFactory::NewLabel();
  // in regalloc/rewrite we may do the same thing again
  // therefore, we can just delay it to regalloc

  //if reach main level, we dont need to keep finding upside
  // next:codegen callexp
  auto s = level->parent_ ? "" : "1";
  auto string_frag = new frame::StringFrag(pointer_map_label,s);
  frags->PushBack(string_frag);
  return string_frag;
}


Level::Level(Level *parent,temp::Label *name, std::list<bool> *formals,std::list<bool> *is_pointer):parent_(parent){

  //main level don't need static link
  if(formals){
    formals->push_front(true);

    //static link stores stack addr,not heap addr
    is_pointer->push_front(false);
  }
  // use x64 frame  
  frame_ = new frame::X64Frame(name,formals,is_pointer);
}


class Cx {
public:
  //patchlist store the addr of some nodes of stm
  //need to call Dopatch
  PatchList trues_;
  PatchList falses_;
  tree::Stm *stm_;

  Cx(PatchList trues, PatchList falses, tree::Stm *stm)
      : trues_(trues), falses_(falses), stm_(stm) {}
};

// tr::Exp != tree::Exp
class Exp {
public:
  [[nodiscard]] virtual tree::Exp *UnEx() = 0;
  [[nodiscard]] virtual tree::Stm *UnNx() = 0;
  [[nodiscard]] virtual Cx UnCx(err::ErrorMsg *errormsg) = 0;
};

class ExpAndTy {
public:
  tr::Exp *exp_;
  type::Ty *ty_;

  ExpAndTy(tr::Exp *exp, type::Ty *ty) : exp_(exp), ty_(ty) {}
};

class ExExp : public Exp {
public:
  tree::Exp *exp_;

  explicit ExExp(tree::Exp *exp) : exp_(exp) {}

  [[nodiscard]] tree::Exp *UnEx() override { 
    /* TODO: Put your lab5 code here */
    return exp_;
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(exp_);
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* TODO: Put your lab5 code here */
    //!=0 true  ==0 false
    auto stm = new tree::CjumpStm(tree::RelOp::NE_OP, exp_, new tree::ConstExp(0), nullptr, nullptr);
    PatchList trues{{&stm->true_label_}};
    PatchList falses{{&stm->false_label_}};
    return {trues,falses,stm};
  }
};

class NxExp : public Exp {
public:
  tree::Stm *stm_;

  explicit NxExp(tree::Stm *stm) : stm_(stm) {}

  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */
    return new tree::EseqExp(stm_, new tree::ConstExp(0));
  }
  [[nodiscard]] tree::Stm *UnNx() override { 
    /* TODO: Put your lab5 code here */
    return stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* TODO: Put your lab5 code here */
    //error
    return {{},{},stm_};
  }
};

class CxExp : public Exp {
public:
  Cx cx_;

  CxExp(PatchList trues, PatchList falses, tree::Stm *stm)
      : cx_(trues, falses, stm) {}
  
  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */
    temp::Temp *r = temp::TempFactory::NewTemp();
    temp::Label *t = temp::LabelFactory::NewLabel();
    temp::Label *f = temp::LabelFactory::NewLabel();
    cx_.trues_.DoPatch(t);
    cx_.falses_.DoPatch(f);
    return new tree::EseqExp(
        new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(1)),
          new tree::EseqExp(
            cx_.stm_,
            new tree::EseqExp(
              // if false return f=0
              new tree::LabelStm(f),
              new tree::EseqExp(
                new tree::MoveStm(new tree::TempExp(r),new tree::ConstExp(0)),
                  //if true return r=1
                  new tree::EseqExp(new tree::LabelStm(t),new tree::TempExp(r))))));

  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* TODO: Put your lab5 code here */
    //cant return cx_.stm_ because have not patch
    return new tree::ExpStm(UnEx());
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override { 
    /* TODO: Put your lab5 code here */
    return cx_;
  }
};


inline tree::Exp *staticLink(tr::Level *level_now,tr::Level *level_target){
  tree::Exp * framePtr = new tree::TempExp(reg_manager->FramePointer());
  while(level_now != level_target){
    //only main dont have static link formal
    framePtr = level_now->frame_->StaticLink()->ToExp(framePtr);
    level_now = level_now->parent_;
  }
  return framePtr;
}

inline tree::Stm *list2tree(std::list<tree::Stm*> stm_list){
  tree::Stm* stm = nullptr;
  for(auto it = stm_list.rbegin();it!=stm_list.rend();it++){
    if(!*it){
      continue;
    }
    if(stm){
      stm = new tree::SeqStm(*it,stm);
    }else{
      stm = *it;
    }
  }
  return stm;
}

void ProgTr::Translate() {
  /* TODO: Put your lab5 code here */
  FillBaseVEnv();
  FillBaseTEnv();
  auto main = absyn_tree_->Translate(venv_.get(),tenv_.get(),main_level_.get(),nullptr,nullptr,errormsg_.get());
  auto frag = new frame::ProcFrag(tr::list2tree(frame::ProcEntryExit1(main_level_->frame_,main->exp_->UnNx())),main_level_->frame_);

  frags->PushBack(frag);
}

} // namespace tr

namespace absyn {

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *break_label, temp::Label *return_label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return root_->Translate(venv,tenv,level,break_label,return_label,errormsg);
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *break_label, temp::Label *return_label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto entry = static_cast<env::VarEntry*>(venv->Look(sym_));
  if(!entry){
    return nullptr;
  }
  //entry must not be null in type checking
  auto access = entry->access_->access_;
  tree::Exp *framePtr = nullptr;
  if(typeid(*access)==typeid(frame::InFrameAccess)){
      // auto target = entry->access_->level_;
      // framePtr = new tree::TempExp(reg_manager->FramePointer());
      // auto l = level;
      // while(l != target){
      //   //only main dont have static link formal
      //   framePtr = l->frame_->StaticLink()->ToExp(framePtr);
      //   l = l->parent_;
      // }
      framePtr = tr::staticLink(level,entry->access_->level_);
  }
  // std::cout<<sym_->Name()<<(!!entry->ty_)<<std::endl;
  return new tr::ExpAndTy(
    new tr::ExExp(entry->access_->access_->ToExp(framePtr)),
    entry->ty_ ? entry->ty_->ActualTy():nullptr
  );
}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *break_label, temp::Label *return_label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */

  auto exp_ty = var_->Translate(venv,tenv,level,break_label,return_label,errormsg);
  auto ty = static_cast<type::RecordTy*>(exp_ty->ty_);
  type::Ty* var_ty = nullptr;
  unsigned int offset = 0;
  auto wordsize = reg_manager->WordSize();
  for(const auto &field:ty->fields_->GetList()){
    if(field->name_ == sym_){
      var_ty = field->ty_;
      break;
    }
    offset += wordsize;
  }

  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::MemExp(
        new tree::BinopExp(
          tree::PLUS_OP,
          exp_ty->exp_->UnEx(),
          new tree::ConstExp(offset)
        )
      )
    ),
    var_ty
  );
}


/*
  type intarray = array of int    type:intarray  value:addr
  intarray[10] of 1               arrayexp       value:addr
  var a:= intarray[10] of 1       vardec         mov a, addr
*/
tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *break_label, temp::Label *return_label,
                                      err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto var_exp_ty = var_->Translate(venv,tenv,level,break_label,return_label,errormsg);
  auto exp_exp_ty = subscript_->Translate(venv,tenv,level,break_label,return_label,errormsg);
  auto base = var_exp_ty->exp_->UnEx();
  auto offset = exp_exp_ty->exp_->UnEx();
  auto size = new tree::ConstExp(reg_manager->WordSize());
  
  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::MemExp(
        new tree::BinopExp(
          tree::PLUS_OP,
          base,
          new tree::BinopExp(
            tree::MUL_OP,
            offset,size
          )
        )
      )
    ),
    static_cast<type::ArrayTy*>(var_exp_ty->ty_)->ty_->ActualTy()
  );
}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *break_label, temp::Label *return_label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto exp_ty = var_->Translate(venv,tenv,level,break_label,return_label,errormsg);

  return new tr::ExpAndTy(
    new tr::ExExp(
      exp_ty->exp_->UnEx()
    ),
    exp_ty->ty_
  );
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *break_label, temp::Label *return_label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::ConstExp(0)
    ),
    type::NilTy::Instance()
  );
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *break_label, temp::Label *return_label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::ConstExp(val_)
    ),
    type::IntTy::Instance()
  );
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *break_label, temp::Label *return_label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */

  auto str_label = temp::LabelFactory::NewLabel();
  frags->PushBack(new frame::StringFrag(str_label,str_));
  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::NameExp(str_label)
    ),
    type::StringTy::Instance()
  );

}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *break_label, temp::Label *return_label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto max_arg_size = reg_manager->ArgRegs()->GetList().size();
  auto exp_list = args_->GetList();
  auto arg_list = new tree::ExpList();
  auto arg_size = exp_list.size();
  auto formal_tylist = new type::TyList();
  for(const auto &exp:exp_list){
    auto arg_exp_ty = exp->Translate(venv,tenv,level,break_label,return_label,errormsg);
    arg_list->Append(arg_exp_ty->exp_->UnEx());
    auto ty = arg_exp_ty->ty_->ActualTy();
    formal_tylist->Append(ty);
  }

  //tell next functiondec how to generate codes
  auto key = formal_tylist->Key();
  if(function2keys.count(func_->Name())){
    function2keys[func_->Name()].emplace(key);
  }else{
    function2keys.emplace(func_->Name(),std::set<uint64_t>{key});
  }


  auto func_entry = static_cast<env::FunEntry*>(venv->Look(func_));

  auto func_label = compile_function ? func_entry->labels_[key]: func_entry->label_;
  auto func_level = compile_function ? func_entry->levels_[key]: func_entry->level_;
  auto string_frag = tr::GetPointerMap(level->frame_,level);
  tree::Exp *call_exp;

  if(func_label){
    //func->entry->level is the level of func itself, parent is the level defines func
    arg_list->Insert(staticLink(level,func_level->parent_));
    call_exp = new tree::CallExp(new tree::NameExp(func_label),arg_list,string_frag,arg_size+1-max_arg_size);
  }else{//env.cc externalcall label=nullptr
    //new NamedLabel
    call_exp = frame::externalCall(func_->Name(),arg_list,string_frag,arg_size-max_arg_size);
  }


  auto res_ty = compile_function ? func_entry->results_[key]: func_entry->result_;

  return new tr::ExpAndTy(
    new tr::ExExp(call_exp),
    res_ty
  );


}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *break_label, temp::Label *return_label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */

  // cjump

  tr::ExpAndTy *left_exp_ty = nullptr;
  if(left_){
    left_exp_ty = left_->Translate(venv,tenv,level,break_label,return_label,errormsg);
  }
  auto right_exp_ty = right_->Translate(venv,tenv,level,break_label,return_label,errormsg);
  {
    tree::BinOp op = tree::BinOp::BIN_OPER_COUNT;
    switch (oper_)
    {
    case PLUS_OP:op = tree::PLUS_OP;break;
    case MINUS_OP:op = tree::MINUS_OP;break;
    case TIMES_OP:op = tree::MUL_OP;break;
    case DIVIDE_OP:op = tree::DIV_OP;break;
    case MOD_OP:op = tree::MOD_OP;break;
    default:
      break;
    }
    if(op != tree::BIN_OPER_COUNT){
      return new tr::ExpAndTy(
        new tr::ExExp(
          new tree::BinopExp(
            op,
            left_exp_ty->exp_->UnEx(),
            right_exp_ty->exp_->UnEx()
          )
        ),
        left_exp_ty->ty_
      );
    }
  }

  {
    tree::RelOp op = tree::RelOp::REL_OPER_COUNT;
    switch (oper_)
    {
    case LT_OP:op = tree::LT_OP;break;
    case LE_OP:op = tree::LE_OP;break;
    case GT_OP:op = tree::GT_OP;break;
    case GE_OP:op = tree::GE_OP;break;
    default:
      break;
    }
    if(op != tree::REL_OPER_COUNT){
      auto cj = new tree::CjumpStm(op,left_exp_ty->exp_->UnEx(),right_exp_ty->exp_->UnEx(),nullptr,nullptr);
      tr::PatchList trues{{&cj->true_label_}};
      tr::PatchList falses{{&cj->false_label_}};
      return new tr::ExpAndTy(
        new tr::CxExp(trues,falses,cj),
        type::IntTy::Instance()
      );
    }
  }

  {
    tree::RelOp op = tree::RelOp::REL_OPER_COUNT;
    switch (oper_)
    {
    case EQ_OP:op = tree::EQ_OP;break;
    case NEQ_OP:op = tree::NE_OP;break;
    default:
      break;
    }
    if(op != tree::REL_OPER_COUNT){
      tree::CjumpStm* cj = nullptr;
      if(compile_function && left_exp_ty->ty_->IsSameType(type::StringTy::Instance())){

        auto str_cmp = frame::externalCall("string_equal",
        new tree::ExpList({left_exp_ty->exp_->UnEx(),right_exp_ty->exp_->UnEx()}),
        tr::GetPointerMap(level->frame_,level)
        );
        //1 eq  0 neq  order does not matter
        cj = new tree::CjumpStm(op,str_cmp,new tree::ConstExp(1),nullptr,nullptr);
      }else{
        cj = new tree::CjumpStm(op,left_exp_ty->exp_->UnEx(),right_exp_ty->exp_->UnEx(),nullptr,nullptr);
      }
      tr::PatchList trues{{&cj->true_label_}};
      tr::PatchList falses{{&cj->false_label_}};
      return new tr::ExpAndTy(
        new tr::CxExp(trues,falses,cj),
        type::IntTy::Instance()
      );
    }
  }

  if(oper_ == AND_OP){
    auto lcx = left_exp_ty->exp_->UnCx(errormsg);
    auto rcx = right_exp_ty->exp_->UnCx(errormsg);
    
    auto right_label = temp::LabelFactory::NewLabel();
    lcx.trues_.DoPatch(right_label);
    auto stm = tr::list2tree({
      lcx.stm_,
      new tree::LabelStm(right_label),
      rcx.stm_
    });
    return new tr::ExpAndTy(
      new tr::CxExp(
        rcx.trues_,
        tr::PatchList::JoinPatch(lcx.falses_,rcx.falses_),
        stm
      ),
      type::IntTy::Instance()
    );
  }

  if(oper_ == OR_OP){
    auto lcx = left_exp_ty->exp_->UnCx(errormsg);
    auto rcx = right_exp_ty->exp_->UnCx(errormsg);
    
    auto right_label = temp::LabelFactory::NewLabel();
    lcx.falses_.DoPatch(right_label);
    auto stm = tr::list2tree({
      lcx.stm_,
      new tree::LabelStm(right_label),
      rcx.stm_
    });
    return new tr::ExpAndTy(
      new tr::CxExp(
        tr::PatchList::JoinPatch(lcx.trues_,rcx.trues_),
        rcx.falses_,
        stm
      ),
      type::IntTy::Instance()
    );
  }
  if(oper_ == NOT_OP){
    auto rcx = right_exp_ty->exp_->UnCx(errormsg);
    return new tr::ExpAndTy(
      new tr::CxExp(
        rcx.falses_,
        rcx.trues_,
        rcx.stm_
      ),
      type::IntTy::Instance()
    );
  }

}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *break_label, temp::Label *return_label,      
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto ty = static_cast<type::RecordTy*>(tenv->Look(typ_));
  auto field_list = ty->fields_->GetList();
  auto wordsize = reg_manager->WordSize();
  auto record_len = field_list.size();

  auto pointer_info = std::string(record_len,'0');
  auto str_label = temp::LabelFactory::NewLabel();
  frags->PushBack(new frame::StringFrag(str_label,pointer_info));

  
  // auto alloc_record = frame::externalCall("alloc_record",new tree::ExpList({new tree::ConstExp(record_len*wordsize)}));

  auto alloc_record = frame::externalCall("alloc_record",
  new tree::ExpList({new tree::NameExp(str_label)}),
  tr::GetPointerMap(level->frame_,level));


  auto r = temp::TempFactory::NewTemp();
  auto move_addr_to_r = new tree::MoveStm(
    new tree::TempExp(r),
    alloc_record
  );

  auto offset = (record_len-1)*wordsize;
  auto efield_list = fields_->GetList();
  auto efield_it = efield_list.rbegin();

  tree::Stm * stm = nullptr;
  for(auto efield_it = efield_list.rbegin();efield_it!=efield_list.rend();efield_it++){
    auto exp_ty = (*efield_it)->exp_->Translate(venv,tenv,level,break_label,return_label,errormsg);
    
    auto move = new tree::MoveStm(
                  new tree::MemExp(
                    new tree::BinopExp(
                      tree::PLUS_OP,
                      new tree::TempExp(r),
                      new tree::ConstExp(offset)
                    )
                  ),
                  exp_ty->exp_->UnEx()
                );
    offset -= wordsize;
    if(stm){
      stm = new tree::SeqStm(move,stm);
    }else{
      stm = move;
    }
  }

  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::EseqExp(
        new tree::SeqStm(
          move_addr_to_r,
          stm
        ),
        new tree::TempExp(r)
      )
    ),
    ty
  );
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *break_label, temp::Label *return_label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto exp_list = seq_->GetList();
  auto exp_it = exp_list.begin();
  std::list<tree::Stm*> stm_list;
  for(;std::next(exp_it)!=exp_list.end();exp_it++){
    auto exp_ty = (*exp_it)->Translate(venv,tenv,level,break_label,return_label,errormsg);
    stm_list.push_back(exp_ty->exp_->UnNx());
  }
  auto exp_ty = (*exp_it)->Translate(venv,tenv,level,break_label,return_label,errormsg);
  if(stm_list.empty()){
    return exp_ty;
  }
  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::EseqExp(
        tr::list2tree(stm_list),
        exp_ty->exp_->UnEx()
      )
    ),
    exp_ty->ty_
  );
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *break_label, temp::Label *return_label,                       
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto var_exp_ty = var_->Translate(venv,tenv,level,break_label,return_label,errormsg);
  auto exp_exp_ty = exp_->Translate(venv,tenv,level,break_label,return_label,errormsg);
  if(var_exp_ty){
      return new tr::ExpAndTy(
        new tr::NxExp(
          new tree::MoveStm(
            var_exp_ty->exp_->UnEx(),
            exp_exp_ty->exp_->UnEx()
          )
        ),
        type::VoidTy::Instance()
      );
  }
  auto access = tr::Access::AllocLocal(level,false,type::IsPointer(exp_exp_ty->ty_));
  venv->Enter(((SimpleVar*)var_)->sym_,new env::VarEntry(access,exp_exp_ty->ty_));
  return new tr::ExpAndTy(
    new tr::NxExp(
      new tree::MoveStm(
        access->access_->ToExp(new tree::TempExp(reg_manager->FramePointer())),
        exp_exp_ty->exp_->UnEx()
      )
    ),
    type::VoidTy::Instance()
  );
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *break_label, temp::Label *return_label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */

  //can be optimized  p117
  
  auto test_exp_ty = test_->Translate(venv,tenv,level,break_label,return_label,errormsg);
  auto then_exp_ty = then_->Translate(venv,tenv,level,break_label,return_label,errormsg);

  auto t = temp::LabelFactory::NewLabel();
  auto f = temp::LabelFactory::NewLabel();
  auto done = temp::LabelFactory::NewLabel();

  auto cx = test_exp_ty->exp_->UnCx(errormsg);
  cx.trues_.DoPatch(t);
  cx.falses_.DoPatch(f);

  if(!elsee_){//if then
    auto stm = tr::list2tree({
      cx.stm_,
      new tree::LabelStm(t),
      then_exp_ty->exp_->UnNx(),
      new tree::LabelStm(f)
    });
    return new tr::ExpAndTy(
      new tr::NxExp(stm),
      type::VoidTy::Instance()
    );
  }
  // if then else
  auto r = temp::TempFactory::NewTemp();
  auto else_exp_ty = elsee_->Translate(venv,tenv,level,break_label,return_label,errormsg);
  auto stm = tr::list2tree({
    cx.stm_,
    new tree::LabelStm(t),
    new tree::MoveStm(
      new tree::TempExp(r),
      then_exp_ty->exp_->UnEx()
    ),
    new tree::JumpStm(new tree::NameExp(done),new std::vector<temp::Label*>{done}),
    new tree::LabelStm(f),
    new tree::MoveStm(
      new tree::TempExp(r),
      else_exp_ty->exp_->UnEx()
    ),
    new tree::LabelStm(done)
  });
  
  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::EseqExp(
        stm,
        new tree::TempExp(r)
      )
    ),
    then_exp_ty->ty_
  );
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *break_label, temp::Label *return_label,            
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto test = temp::LabelFactory::NewLabel();
  auto done = temp::LabelFactory::NewLabel();
  auto body = temp::LabelFactory::NewLabel();
  auto test_exp_ty = test_->Translate(venv,tenv,level,break_label,return_label,errormsg);
  auto body_exp_ty = body_->Translate(venv,tenv,level,done,return_label,errormsg);
  auto cx = test_exp_ty->exp_->UnCx(errormsg);
  cx.trues_.DoPatch(body);
  cx.falses_.DoPatch(done);

  auto seq = tr::list2tree({
    new tree::LabelStm(test),
    cx.stm_,
    new tree::LabelStm(body),
    body_exp_ty->exp_->UnNx(),
    new tree::JumpStm(new tree::NameExp(test),new std::vector<temp::Label*>{test}),
    new tree::LabelStm(done)
  });
  return new tr::ExpAndTy(
    new tr::NxExp(
      seq
    ),
    type::VoidTy::Instance()
  );
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *break_label, temp::Label *return_label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto low_exp_ty = lo_->Translate(venv,tenv,level,break_label,return_label,errormsg);
  auto high_exp_ty = hi_->Translate(venv,tenv,level,break_label,return_label,errormsg);

  venv->BeginScope();
  auto access = tr::Access::AllocLocal(level,escape_);
  auto access_limit = tr::Access::AllocLocal(level,false);
  auto i = access->access_->ToExp(new tree::TempExp(reg_manager->FramePointer()));
  auto limit = access_limit->access_->ToExp(nullptr);


  auto body = temp::LabelFactory::NewLabel();
  auto loop = temp::LabelFactory::NewLabel();
  auto done = temp::LabelFactory::NewLabel();
  auto test = temp::LabelFactory::NewLabel();
  venv->Enter(var_,new env::VarEntry(access,low_exp_ty->ty_,true));

  auto body_exp_ty = body_->Translate(venv,tenv,level,done,return_label,errormsg);

  venv->EndScope();


  // if consider maxint  one of two body will be eliminated by canon???
  
  // auto cj1 = new tree::CjumpStm(tree::GT_OP,i,limit,done,body);
  // auto cj2 = new tree::CjumpStm(tree::EQ_OP,i,limit,done,loop);
  // auto cj3 = new tree::CjumpStm(tree::LT_OP,i,limit,loop,done);

  auto cj = new tree::CjumpStm(tree::LT_OP,i,limit,body,done);

  auto body_stm = body_exp_ty->exp_->UnNx();
  auto seq = tr::list2tree({
    new tree::MoveStm(
      i,low_exp_ty->exp_->UnEx()
    ),
    new tree::MoveStm(
      limit,high_exp_ty->exp_->UnEx()
    ),
    // cj1,
    new tree::LabelStm(test),
    cj,
    new tree::LabelStm(body),
    body_stm,
    // cj2,
    // new tree::LabelStm(loop),
    new tree::MoveStm(
      i,
      new tree::BinopExp(tree::PLUS_OP,i,new tree::ConstExp(1))
    ),
    // body_stm,
    // cj3,
    new tree::JumpStm(new tree::NameExp(test),new std::vector<temp::Label*>{test}),
    new tree::LabelStm(done)
  });
  return new tr::ExpAndTy(
    new tr::NxExp(seq),
    type::VoidTy::Instance()
  );
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *break_label, temp::Label *return_label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new::tr::ExpAndTy(
    new tr::NxExp(
      new tree::JumpStm(
        new tree::NameExp(break_label),new std::vector<temp::Label*>{break_label}
      )
    ),
    type::VoidTy::Instance()
  );

}



tr::ExpAndTy *ReturnExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *break_label, temp::Label *return_label,
                                  err::ErrorMsg *errormsg) const {
  auto res_exp_ty = ret_->Translate(venv,tenv,level,break_label,return_label,errormsg);
  return new::tr::ExpAndTy(
    new tr::NxExp(
      tr::list2tree({
        new tree::MoveStm(
          new tree::TempExp(reg_manager->ReturnValue()),
          res_exp_ty->exp_->UnEx()
        ),
        new tree::JumpStm(
          new tree::NameExp(return_label),new std::vector<temp::Label*>{return_label}
        )
      })
    ),
    type::VoidTy::Instance()
  );

}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *break_label, temp::Label *return_label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */

  if (!body_){
    return new tr::ExpAndTy(
      new tr::ExExp(new tree::ConstExp(0)),
      type::VoidTy::Instance()
    );
  }

  venv->BeginScope();
  tenv->BeginScope();
  auto decslist = decs_->GetList();
  std::list<tree::Stm*> dec_stm_list;
  for (const auto &dec : decslist){
    auto dec_exp = dec->Translate(venv, tenv, level,break_label,return_label, errormsg);
    dec_stm_list.push_back(dec_exp->UnNx());
  }

  auto body_exp_ty = body_->Translate(venv, tenv, level,break_label,return_label, errormsg);
  
  tenv->EndScope();
  venv->EndScope();

  auto seqstm = tr::list2tree(dec_stm_list);
  if(!seqstm){
    return body_exp_ty;
  }
  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::EseqExp(
        seqstm,
        body_exp_ty->exp_->UnEx()
      )
    ),
    body_exp_ty->ty_
  );

}

tr::ExpAndTy *FunctionExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *break_label, temp::Label *return_label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */

  // venv->BeginScope();
  // tenv->BeginScope();
  auto dec_exp = funcs_->Translate(venv, tenv, level,break_label,return_label, errormsg);
  // tenv->EndScope();
  // venv->EndScope();
  return new tr::ExpAndTy(
    new tr::NxExp(
      dec_exp->UnNx()
    ),
    type::VoidTy::Instance()
  );

}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *break_label, temp::Label *return_label,                    
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto ty = static_cast<type::ArrayTy*>(tenv->Look(typ_));
  auto size_exp_ty = size_->Translate(venv,tenv,level,break_label,return_label,errormsg);
  auto init_exp_ty = init_->Translate(venv,tenv,level,break_label,return_label,errormsg);

  auto init_array = frame::externalCall("init_array",
  new tree::ExpList({size_exp_ty->exp_->UnEx(),init_exp_ty->exp_->UnEx()}),
  tr::GetPointerMap(level->frame_,level));

  //externalcall already mov init value
  return new tr::ExpAndTy(
    new tr::ExExp(
      init_array
    ),
    ty
  );
  
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *break_label, temp::Label *return_label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  
  return new tr::ExpAndTy(
    new tr::NxExp(
      nullptr
    ),
    type::VoidTy::Instance()
  );
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *break_label, temp::Label *return_label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto func_list = functions_->GetList();

  for(const auto&function:func_list){
    auto params = function->params_;
    auto escape = new std::list<bool>;
    //dont add static link here ,Level will add it in constructor
    for(const auto&arg:params->GetList()){
      escape->push_back(arg->escape_);
    }
    type::Ty *result_ty = type::VoidTy::Instance();
    if(function->result_){
      result_ty = tenv->Look(function->result_); 
    }
    auto formals = params->MakeFormalTyList(tenv, errormsg);
    auto is_pointer = new std::list<bool>;
    //TODO is_pointer if compile_function
    for(const auto&param:params->GetList()){
      auto ty = tenv->Look(param->typ_);
      is_pointer->push_back(type::IsPointer(ty));
    }
    auto name = function->name_;
    if(!compile_function){
      auto f_label = temp::LabelFactory::NamedLabel(name->Name());
      auto f_level = new tr::Level(level,f_label,escape,is_pointer);
      //must add level of the function to env instead of the level defines it
      venv->Enter(name,new env::FunEntry(f_level,f_label,formals,result_ty));
    }else{
      auto keys = function2keys[name->Name()];
      for(auto key:keys){
        auto f_label = temp::LabelFactory::NamedLabel(name->Name()+"_"+std::to_string(key));
        auto f_level = new tr::Level(level,f_label,escape,is_pointer);
        auto entry = static_cast<env::FunEntry*>(venv->Look(name));
        if(!entry){
          venv->Enter(name,new env::FunEntry(
            std::map<uint64_t,tr::Level*>{{key,f_level}},
            std::map<uint64_t,temp::Label*>{{key,f_label}},
            std::map<uint64_t,type::TyList*>{{key,type::key2List(key)}},
            std::map<uint64_t,type::Ty*>{{key,result_ty}}
            ));
        }else{
          entry->Append(key,f_level,f_label,type::key2List(key),result_ty);
        }
      }
    }
  }
  for(const auto&function:func_list){
    if(!compile_function){
      auto entry = static_cast<env::FunEntry*>(venv->Look(function->name_));
      auto f_level = entry->level_;
      auto params = function->params_;
      auto formals = params->MakeFormalTyList(tenv, errormsg);
      venv->BeginScope();

      //after new frame,params will be allocated
      auto formal_it = entry->level_->frame_->GetFormalList().begin();
      formal_it++;//static link
      // auto ty_it = entry->formals_->GetList().begin();//not contain static link
      auto param_it = params->GetList().begin();
      for (; param_it != params->GetList().end(); formal_it++, param_it++){
        //this access shows the level of where formal defines is the same as the function
        venv->Enter((*param_it)->name_,new env::VarEntry(new tr::Access(f_level,*formal_it),nullptr));
      }
      

      auto ret_label = temp::LabelFactory::NewLabel();
      auto res_exp_ty = function->body_->Translate(venv,tenv,f_level,break_label,ret_label,errormsg);
      venv->EndScope();

      auto ret = tr::list2tree({
        new tree::MoveStm(
          new tree::TempExp(reg_manager->ReturnValue()),
          res_exp_ty->exp_->UnEx()
        ),
        new tree::LabelStm(ret_label)
      });

      auto frag = new frame::ProcFrag(tr::list2tree(frame::ProcEntryExit1(f_level->frame_,ret)),f_level->frame_);
      frags->PushBack(frag);
    }else{
      auto keys = function2keys[function->name_->Name()];
      for(auto key:keys){
        auto entry = static_cast<env::FunEntry*>(venv->Look(function->name_));
        auto f_level = entry->levels_[key];
        auto params = function->params_;
        auto formals = entry->formalss_[key];
        venv->BeginScope();

        //after new frame,params will be allocated
        auto formal_it = f_level->frame_->GetFormalList().begin();
        formal_it++;//static link
        auto ty_it = formals->GetList().begin();//not contain static link
        auto param_it = params->GetList().begin();
        for (; param_it != params->GetList().end(); formal_it++, param_it++,ty_it++){
          //this access shows the level of where formal defines is the same as the function
          // std::cout<<"enter"<<(*param_it)->name_->Name()<<(!!*ty_it)<<std::endl;
          venv->Enter((*param_it)->name_,new env::VarEntry(new tr::Access(f_level,*formal_it),*ty_it));
        }
        

        auto ret_label = temp::LabelFactory::NewLabel();
        auto res_exp_ty = function->body_->Translate(venv,tenv,f_level,break_label,ret_label,errormsg);
        venv->EndScope();

        auto ret = tr::list2tree({
          new tree::MoveStm(
            new tree::TempExp(reg_manager->ReturnValue()),
            res_exp_ty->exp_->UnEx()
          ),
          new tree::LabelStm(ret_label)
        });

        auto frag = new frame::ProcFrag(tr::list2tree(frame::ProcEntryExit1(f_level->frame_,ret)),f_level->frame_);
        frags->PushBack(frag);
      }
    }
  }

  return NOP;
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *break_label, temp::Label *return_label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto init_exp_ty = init_->Translate(venv,tenv,level,break_label,return_label,errormsg);
  decltype(init_exp_ty->ty_) ty;
  if(typ_){
    ty = tenv->Look(typ_);
  }else{
    ty = init_exp_ty->ty_;
  }
  auto access = tr::Access::AllocLocal(level,escape_,type::IsPointer(ty));
  venv->Enter(var_,new env::VarEntry(access,ty));

  return new tr::NxExp(
    new tree::MoveStm(
      access->access_->ToExp(
        new tree::TempExp(
          reg_manager->FramePointer()
          )
        ),
      init_exp_ty->exp_->UnEx()
    )
  );
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *break_label, temp::Label *return_label,
                            err::ErrorMsg *errormsg) const {
 /* TODO: Put your lab5 code here */
  auto type_list = types_->GetList();
  for(const auto &type:type_list){
    tenv->Enter(type->name_, new type::NameTy(type->name_,nullptr)); 
  }
  for(const auto &type:type_list){
    
    auto ty = type->ty_->Translate(tenv, errormsg);
    tenv->Enter(type->name_, ty); 
    //type list = { first: int, rest: list }
    if(typeid(*ty)==typeid(type::RecordTy)){
      auto fields_list = static_cast<type::RecordTy*>(ty)->fields_->GetList();
      for(auto&field:fields_list){
          if(typeid(*field->ty_)==typeid(type::NameTy)){
            auto ty_ = static_cast<type::NameTy*>(field->ty_);
            if(ty_->sym_ == type->name_ && !ty_->ty_){
              field->ty_ = ty;
            }
          }
      }
    }
  }

  return NOP;
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new type::NameTy(
    name_,tenv->Look(name_)
  );
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new type::RecordTy(
    record_->MakeFieldList(tenv,errormsg)
  );
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new type::ArrayTy(
    tenv->Look(array_)
  );
}

} // namespace absyn
