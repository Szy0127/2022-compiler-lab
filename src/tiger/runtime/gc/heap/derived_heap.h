#pragma once

#include "heap.h"
#include <vector>
#include <map>


namespace gc {

class Descriptor{
  public:
  Descriptor()=default;
  [[nodiscard]] virtual unsigned int GetSize()const=0;
};

class RecordDescriptor:public Descriptor{
  public:
  RecordDescriptor(std::vector<bool> is_pointer):_is_pointer(std::move(is_pointer)){}
  unsigned int GetSize()const override{return _is_pointer.size()*TigerHeap::WORD_SIZE;}
  [[nodiscard]]  const std::vector<bool> &GetInfo()const{return _is_pointer;}
  private:
  std::vector<bool> _is_pointer;
};
class ArrayDescriptor:public Descriptor{
  public:
  ArrayDescriptor(unsigned int slot_num,bool is_pointer):_slot_num(slot_num),_is_pointer(is_pointer){}
  unsigned int GetSize()const override{return _slot_num*TigerHeap::WORD_SIZE;}
  [[nodiscard]] bool IsPointer()const{return _is_pointer;}
  private:
  unsigned int _slot_num;
  bool _is_pointer;
};
class DerivedHeap : public TigerHeap {
  // TODO(lab7): You need to implement those interfaces inherited from TigerHeap correctly according to your design.
public:
  DerivedHeap()=default;
  char *Allocate(uint64_t size)override;

  char *Allocate(std::string pointer_info);
  char *Allocate(uint64_t slot_number,bool is_pointer);


  uint64_t Used() const override;


  uint64_t MaxFree() const override;


  void Initialize(uint64_t size) override;

  void GC() override;

private:
  char* _heap;
  char* _heap_end;
  char* _from_space;
  char* _to_space;
  uint64_t from_offset;

  std::map<uint64_t,Descriptor*> addr2desc;
};

} // namespace gc

