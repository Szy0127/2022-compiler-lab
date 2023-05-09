#include "tiger/semant/types.h"

namespace type {

NilTy NilTy::nilty_;
IntTy IntTy::intty_;
StringTy StringTy::stringty_;
VoidTy VoidTy::voidty_;
ListTy ListTy::listty_;

Ty *Ty::ActualTy() { return this; }

Ty *NameTy::ActualTy() {
  assert(ty_ != this);
  return ty_->ActualTy();
}

bool Ty::IsSameType(Ty *expected) {
  Ty *a = ActualTy();
  Ty *b = expected->ActualTy();

  if ((typeid(*a) == typeid(NilTy) && typeid(*b) == typeid(RecordTy)) ||
      (typeid(*a) == typeid(RecordTy) && typeid(*b) == typeid(NilTy)))
    return true;

  return a == b;
}

bool IsPointer(Ty *ty){
  if(!ty){
    return false;
  }
  ty = ty->ActualTy();
  return typeid(*ty) == typeid(type::ArrayTy) || typeid(*ty) == typeid(type::RecordTy) || typeid(*ty) == typeid(type::NilTy);
}

uint64_t KeyOfType(Ty *ty){
  Ty *a = ty->ActualTy();
  if(typeid(*a) == typeid(NilTy)){
    return 1;
  }
  if(typeid(*a) == typeid(IntTy)){
    return 2;
  }
  if(typeid(*a) == typeid(StringTy)){
    return 3;
  }
  if(typeid(*a) == typeid(VoidTy)){
    return 4;
  }
  if(typeid(*a) == typeid(RecordTy)){
    return 5;
  }
  if(typeid(*a) == typeid(ListTy)){
    return 6;
  }
  return 7;
}

uint64_t TyList::Key()const{
  uint64_t key = 0;
  for(const auto &ty:ty_list_){
    key = key * 10 + KeyOfType(ty);
  }
  return key;
}

type::TyList *key2List(uint64_t key){
  auto list = new type::TyList();
  while(key){
    uint64_t k = key % 10;
    key /= 10;
    switch(k){
      case 1:
        list->PushFront(type::NilTy::Instance());
        break;
      case 2:
        list->PushFront(type::IntTy::Instance());
        break;
      case 3:
        list->PushFront(type::StringTy::Instance());
        break;
      case 4:
        list->PushFront(type::VoidTy::Instance());
        break;
      case 5:
        list->PushFront(new type::RecordTy(nullptr));
        break;
      case 6:
        list->PushFront(type::ListTy::Instance());
        break;
      default:
        assert(false);
    }
  }
}


} // namespace type
