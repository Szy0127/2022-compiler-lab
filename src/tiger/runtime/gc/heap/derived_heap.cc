#include "derived_heap.h"
#include <stdio.h>
#include <stack>
#include <cstring>
#include "../roots/roots.h"

namespace gc {
// TODO(lab7): You need to implement those interfaces inherited from TigerHeap correctly according to your design.


char *DerivedHeap::Allocate(uint64_t size){
    auto ret = _from_space + from_offset;
    // fprintf(stdout,"record addr:%#llx,size:%d\n",ret,size);
    if((_from_space < _to_space && ret+size >= _to_space) || (_from_space > _to_space && ret+size >= _heap_end)){
        return nullptr;
    }
    from_offset += size;
    return ret;
}

char *DerivedHeap::Allocate(std::string pointer_info){
    auto ret = Allocate(pointer_info.size()*WORD_SIZE);
    if(!ret){
        return ret;
    }
    std::vector<bool> v;
    for(const auto&c:pointer_info){
        if(c=='1'){
            v.push_back(true);
        }else{
            v.push_back(false);
        }
    }
    auto desc = new RecordDescriptor(v);
    addr2desc.emplace((uint64_t)ret,desc);

    uint64_t* rsp;
    GET_TIGER_STACK(rsp);
    auto roots_finder = Roots(rsp);
    roots_finder.FindRoots();
    return ret;
}
char *DerivedHeap::Allocate(uint64_t slot_number,bool is_pointer){
    auto ret = Allocate(slot_number*WORD_SIZE);
    if(!ret){
        return ret;
    }
    auto desc = new ArrayDescriptor(slot_number,is_pointer);
    addr2desc.emplace((uint64_t)ret,desc);
    return ret;
}


uint64_t DerivedHeap::Used()const{
    return from_offset;
}


uint64_t DerivedHeap::MaxFree()const{
    return (_heap_end-_heap)/2 - from_offset;
}


void DerivedHeap::Initialize(uint64_t size){
    _heap = new char[size];
    _heap_end = _heap + size;
    _from_space = _heap;
    _to_space = _heap + size/2;
    from_offset = 0;
}

void DerivedHeap::GC(){
    // uint64_t *sp;
    // GET_TIGER_STACK(sp);
    // std::cout<<"sp:"<<sp<<std::endl;

    //tigerfunc -> alloc --> GC
    //cant use other functions to find roots
}
} // namespace gc

