#ifndef TIGER_RUNTIME_GC_ROOTS_H
#define TIGER_RUNTIME_GC_ROOTS_H

#include <iostream>
#include <list>
#include <sstream>

#define WORD_SIZE 8
namespace gc {

struct string {
  int length;
  unsigned char chars[1];
};

class Roots {
  // Todo(lab7): define some member and methods here to keep track of gc roots;
public:
  Roots(uint64_t *rsp):_rsp(rsp){}

  void FindRoots();

  [[nodiscard]] const std::list<uint64_t>  &GetRoots()const{return _pointers;}

private:
  void _findRoots(struct string* pointer_map,bool &last,int &framesize);
private:
  uint64_t* _rsp;
  std::list<uint64_t> _pointers;//value of pointer actually uint*
};

void Roots::_findRoots(struct string* pointer_map,bool &last,int &framesize){
  std::string pointer_map_data;
  pointer_map_data.assign(pointer_map->chars,pointer_map->chars+pointer_map->length);

  std::stringstream ss(pointer_map_data);
  int frame_size;
  ss >> frame_size;
  last = frame_size < 0;
  if(last){
      frame_size  = -frame_size;
  }
  framesize = frame_size;
  // fprintf(stdout,"%d\n",frame_size);

  std::list<uint32_t> offset;
  int off;
  while(ss>>off){
    offset.push_back(off);
  }
  int reg_size = offset.back();
  offset.pop_back();

  // fprintf(stdout,":%d,%d\n",reg_size,offset.size());

  //find reg values;
  for(auto i = 0; i < reg_size;i++){
    auto reg_value = *(uint64_t*)((uint64_t)_rsp+(i+2)*WORD_SIZE);
    // fprintf(stdout,"reg value:%#llx\n",reg_value);
    _pointers.push_back(reg_value);
  }

  //find stack slot values
  for(const auto&off:offset){
    auto stack_value = *(uint64_t*)((uint64_t)_rsp+WORD_SIZE+frame_size-off);
    // fprintf(stdout,"stack value:%#llx\n",stack_value);
    _pointers.push_back(stack_value);
  }
}
void Roots::FindRoots(){
  auto sp = (uint64_t)_rsp;

  // fprintf(stdout,"findroots\n");
  bool last = false;
  int frame_size;
  while(!last){
    /*
        pointer map
        retaddr <--rsp
    */
    auto pointer_map = (struct string*)*(uint64_t*)(sp+WORD_SIZE);
    // fprintf(stdout,"%s\n",pointer_map->chars);
    _findRoots(pointer_map,last,frame_size);
    sp += frame_size+WORD_SIZE;//this wordsize is for retaddr,which dont belong to any counting in previous code
  }

}

}

#endif // TIGER_RUNTIME_GC_ROOTS_H