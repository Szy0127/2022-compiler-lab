#ifndef TIGER_FRAME_TEMP_H_
#define TIGER_FRAME_TEMP_H_

#include "tiger/symbol/symbol.h"

#include <list>

namespace temp {

using Label = sym::Symbol;

class LabelFactory {
public:
  static Label *NewLabel();
  static Label *NamedLabel(std::string_view name);
  static std::string LabelString(Label *s);

private:
  int label_id_ = 0;
  static LabelFactory label_factory;
};

class Temp {//virtual registers
  friend class TempFactory;

public:
  [[nodiscard]] int Int() const;
  [[nodiscard]] bool IsPointer()const;
  [[nodiscard]] bool IsDouble()const;
private:
  int num_;
  bool is_pointer_;
  bool is_double_;
  explicit Temp(int num,bool is_pointer,bool is_double=false) : num_(num),is_pointer_(is_pointer),is_double_(is_double) {}
};

class TempFactory {
public:
  static Temp *NewTemp(bool is_pointer=false,bool is_double=false);

private:
  int temp_id_ = 100;
  static TempFactory temp_factory;
};

class Map {
public:
  void Enter(Temp *t, std::string *s);
  std::string *Look(Temp *t);
  void DumpMap(FILE *out);

  static Map *Empty();
  static Map *Name();
  static Map *LayerMap(Map *over, Map *under);

private:
  tab::Table<Temp, std::string> *tab_;
  Map *under_;

  Map() : tab_(new tab::Table<Temp, std::string>()), under_(nullptr) {}
  Map(tab::Table<Temp, std::string> *tab, Map *under)
      : tab_(tab), under_(under) {}
};

class TempList {
public:
  explicit TempList(Temp *t) : temp_list_({t}) {}
  TempList(std::initializer_list<Temp *> list) : temp_list_(list) {}
  TempList() = default;
  void Append(Temp *t) { temp_list_.push_back(t); }
  bool Empty()const{return temp_list_.empty();}
  bool Contain(Temp *target){
    for (auto t : temp_list_) {
      if (t == target)
        return true;
    }
    return false;
  }
  void Replace(Temp *t1,Temp*t2){
    //remove t1,add t2
    auto it = temp_list_.begin();
    while(true){
      if(*it == t1){
        it = temp_list_.erase(it);
        it  = temp_list_.insert(it,t2);
        if(it==temp_list_.end()){
          break;
        }
      }
      it++;
      if(it==temp_list_.end()){
        break;
      }
    }
  }
  [[nodiscard]] Temp *NthTemp(int i) const;
  [[nodiscard]] const std::list<Temp *> &GetList() const { return temp_list_; }

private:
  std::list<Temp *> temp_list_;
};

} // namespace temp

#endif