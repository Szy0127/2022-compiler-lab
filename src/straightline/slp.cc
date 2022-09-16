#include "straightline/slp.h"

#include <iostream>

namespace A {
int A::CompoundStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  auto l = stm1->MaxArgs();
  auto r = stm2->MaxArgs();
  return l > r ? l : r;
}

Table *A::CompoundStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  t = stm1->Interp(t);
  return stm2->Interp(t);
}

int A::AssignStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return exp->MaxArgs();
}

Table *A::AssignStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  auto intAndTable = exp->Interp(t);
  auto res = intAndTable->t->Update(id,intAndTable->i);
  delete intAndTable;
  return res;
}

int A::PrintStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return exps->NumExps();
}

Table *A::PrintStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  auto res = exps->Interp(t);
  return res->t;
}


int Table::Lookup(const std::string &key) const {
  if (id == key) {
    return value;
  } else if (tail != nullptr) {
    return tail->Lookup(key);
  } else {
    assert(false);
  }
}

Table *Table::Update(const std::string &key, int val) const {
  return new Table(key, val, this);
}


IntAndTable *IdExp::Interp(Table *t) const {
  return new IntAndTable(t->Lookup(id),t);
}


IntAndTable *NumExp::Interp(Table *t) const {
  return new IntAndTable(num,t);
}

IntAndTable *OpExp::Interp(Table *t) const {
  auto res_l = left->Interp(t);
  auto res_r = right->Interp(res_l->t);
  switch (oper)
  {
  case PLUS:
    res_l->i += res_r->i;
    break;
    case MINUS:
    res_l->i -= res_r->i;
    break;
    case TIMES:
    res_l->i *= res_r->i;
    break;
    case DIV:
    res_l->i /= res_r->i;// 0?
    break;
  default:
    break;
  }
  res_l->t = res_r->t;
  delete res_r;
  return res_l;
}

IntAndTable *EseqExp::Interp(Table *t) const {
  t = stm->Interp(t);
  return exp->Interp(t);
}


IntAndTable *PairExpList::Interp(Table *t) const {
  auto intAndTable = exp->Interp(t);
  std::cout<<intAndTable->i<<" ";
  auto res = tail->Interp(intAndTable->t);
  delete intAndTable;
  return res;
}

IntAndTable *LastExpList::Interp(Table *t) const {
  auto res = exp->Interp(t);
  std::cout<<res->i<<std::endl;
  return res;
}

int PairExpList::NumExps() const {
  // std::cout<<tail->NumExps()<<std::endl;
  return tail->NumExps()+1;
}

int LastExpList::NumExps() const {
  return 1;
}


int Exp::MaxArgs() const {
  return 0;
}

int EseqExp::MaxArgs() const {
  return stm->MaxArgs();
}
}  // namespace A




