#ifndef TIGER_ENV_ENV_H_
#define TIGER_ENV_ENV_H_

#include "tiger/frame/temp.h"
#include "tiger/semant/types.h"
#include "tiger/symbol/symbol.h"
#include <map>

// Forward Declarations
namespace tr {
class Access;
class Level;
} // namespace tr

namespace env {
class EnvEntry {
public:
  bool readonly_;

  explicit EnvEntry(bool readonly = true) : readonly_(readonly) {}
  virtual ~EnvEntry() = default;
};

class VarEntry : public EnvEntry {
public:
  tr::Access *access_;
  type::Ty *ty_;

  // For lab4(semantic analysis) only
  explicit VarEntry(type::Ty *ty, bool readonly = false)
      : EnvEntry(readonly), ty_(ty), access_(nullptr){};

  // For lab5(translate IR tree)
  VarEntry(tr::Access *access, type::Ty *ty, bool readonly = false)
      : EnvEntry(readonly), ty_(ty), access_(access){};
};

class FunEntry : public EnvEntry {
public:
  tr::Level *level_;
  temp::Label *label_;
  type::TyList *formals_;
  type::Ty *result_;

  std::map<uint64_t,tr::Level*> levels_;
  std::map<uint64_t,temp::Label*> labels_;
  std::map<uint64_t,type::TyList*> formalss_;
  std::map<uint64_t,type::Ty*> results_;

  // For lab4(semantic analysis) only
  FunEntry(type::TyList *formals, type::Ty *result)
      : formals_(formals), result_(result), level_(nullptr), label_(nullptr) {}

  // For lab5(translate IR tree)
  FunEntry(tr::Level *level, temp::Label *label, type::TyList *formals,
           type::Ty *result)
      : formals_(formals), result_(result), level_(level), label_(label) {}

  FunEntry(std::map<uint64_t,tr::Level*> levels, std::map<uint64_t,temp::Label*> labels, std::map<uint64_t,type::TyList*> formalss,
           std::map<uint64_t,type::Ty*> results)
      : formalss_(formalss), results_(results), levels_(levels), labels_(labels) {}
  void Append(uint64_t key,tr::Level *level, temp::Label *label, type::TyList *formals,
           type::Ty *result){
            if(levels_.count(key)){
              return;
            }
            levels_.emplace(key,level);
            labels_.emplace(key,label);
            formalss_.emplace(key,formals);
            results_.emplace(key,result);
           }
};

using VEnv = sym::Table<env::EnvEntry>;
using TEnv = sym::Table<type::Ty>;
using VEnvPtr = sym::Table<env::EnvEntry> *;
using TEnvPtr = sym::Table<type::Ty> *;
} // namespace env

#endif // TIGER_ENV_ENV_H_
