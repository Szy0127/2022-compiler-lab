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

  [[nodiscard]] const std::list<uint64_t *>  &GetRoots()const{return _pointers;}
private:
  uint64_t* _rsp;
  std::list<uint64_t *> _pointers;
};

void Roots::FindRoots(){
  auto pointer_map = (struct string*)*(uint64_t*)((uint64_t)_rsp+WORD_SIZE);
  // fprintf(stdout,"%s\n",pointer_map->chars);
  std::string pointer_map_data;
  pointer_map_data.assign(pointer_map->chars,pointer_map->chars+pointer_map->length);

  std::stringstream ss(pointer_map_data);
  int frame_size;
  ss >> frame_size;
  bool last = frame_size < 0;
  if(last){
      frame_size  = -frame_size;
  }
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
  }

}

}

#endif // TIGER_RUNTIME_GC_ROOTS_H