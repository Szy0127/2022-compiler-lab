#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>

#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/x64frame.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/frame.h"

extern frame::Frags *frags;
extern frame::RegManager *reg_manager;

namespace tr {

Access *Access::AllocLocal(Level *level, bool escape) {
  /* TODO: Put your lab5 code here */
  return new Access(level,level->frame_->AllocLocal(escape));
}
Level::Level(Level *parent,temp::Label *name, std::list<bool> *formals):parent_(parent){

  //main level don't need static link
  if(formals){
    formals->push_front(true);
  }
  // use x64 frame  
  frame_ = new frame::X64Frame(name,formals);
}

class Cx {
public:
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
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* TODO: Put your lab5 code here */
    //!=0 true  ==0 false
    auto stm = new tree::CjumpStm(tree::RelOp::NE_OP, exp_, new tree::ConstExp(0), nullptr, nullptr);
    PatchList trues{{&stm->true_label_}};
    PatchList falses{{&stm->true_label_}};
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


void ProgTr::Translate() {
  /* TODO: Put your lab5 code here */
  FillBaseVEnv();
  FillBaseTEnv();
  absyn_tree_->Translate(venv_.get(),tenv_.get(),main_level_.get(),nullptr,errormsg_.get());
}

} // namespace tr

namespace absyn {

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  root_->Translate(venv,tenv,level,label,errormsg);
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto entry = static_cast<env::VarEntry*>(venv->Look(sym_));
  //entry must not be null in type checking
  auto access = entry->access_->access_;
  tree::Exp *framePtr = nullptr;
  if(typeid(*access)==typeid(frame::InFrameAccess)){
      auto target = entry->access_->level_;
      framePtr = new tree::TempExp(reg_manager->FramePointer());
      auto l = level->parent_;
      while(l != target){
        //only main dont have static link formal
        framePtr = l->frame_->StaticLink()->ToExp(framePtr);
        l = l->parent_;
      }
  }
  return new tr::ExpAndTy(
    new tr::ExExp(entry->access_->access_->ToExp(framePtr)),
    entry->ty_->ActualTy()
  );
}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */

  auto exp_ty = var_->Translate(venv,tenv,level,label,errormsg);
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
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto var_exp_ty = var_->Translate(venv,tenv,level,label,errormsg);
  auto exp_exp_ty = subscript_->Translate(venv,tenv,level,label,errormsg);
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
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,      
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto ty = static_cast<type::RecordTy*>(tenv->Look(typ_));
  auto field_list = ty->fields_->GetList();
  auto wordsize = reg_manager->WordSize();
  auto record_len = field_list.size();
  auto alloc_record = frame::externalCall("alloc_record",new tree::ExpList({new tree::ConstExp(record_len*wordsize)}));

  auto r = temp::TempFactory::NewTemp();
  auto move_addr_to_r = new tree::MoveStm(
    new tree::TempExp(r),
    alloc_record
  );

  auto offset = record_len-1;
  auto efield_list = fields_->GetList();
  auto efield_it = efield_list.rbegin();

  tree::Stm * stm = nullptr;
  for(auto efield_it = efield_list.rbegin();efield_it!=efield_list.rend();efield_it++){
    auto exp_ty = (*efield_it)->exp_->Translate(venv,tenv,level,label,errormsg);
    
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
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // return tr::ExpAndTy(
  //   new tree::S
  // )
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,                       
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,            
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,                    
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto ty = static_cast<type::ArrayTy*>(tenv->Look(typ_));
  auto size_exp_ty = size_->Translate(venv,tenv,level,label,errormsg);
  auto init_exp_ty = init_->Translate(venv,tenv,level,label,errormsg);
  auto init_array = frame::externalCall("init_array",new tree::ExpList({size_exp_ty->exp_->UnEx(),init_exp_ty->exp_->UnEx()}));

  //externalcall already mov init value
  return new tr::ExpAndTy(
    new tr::ExExp(
      init_array
    ),
    ty
  );
  
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

} // namespace absyn
