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
    if(((uint64_t)_heap == _from_space && ret+size >= _to_space) ||  ret+size >= (uint64_t)_heap_end){
        return nullptr;
    }
    from_offset += size;
    return (char*)ret;
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
    return ret;

    uint64_t* rsp;
    GET_TIGER_STACK(rsp);
    fprintf(stdout,"init root finder\n");
    auto roots_finder = Roots(rsp);
    roots_finder.FindRoots();
    auto roots = roots_finder.GetRoots();
    
    scan = _to_space;
    next = _to_space;
    for(auto&addr_of_p:roots){
        // fprintf(stdout,"%#llx,%d\n",*addr_of_p,*addr_of_p!=0 ? addr2desc[*addr_of_p]->GetSize():0);
        if(*addr_of_p==0){
            continue;
        }
        fprintf(stdout,"before:%#llx\n",*addr_of_p);
        *addr_of_p = Forward(*addr_of_p);
        fprintf(stdout,"after:%#llx\n",*addr_of_p);
    }
    fprintf(stdout,"while\n");
    while(scan < next){
        fprintf(stdout,"scan:%#llx\n",scan);
        auto des = addr2desc[scan];
        fprintf(stdout,"size:%d\n",des->GetSize());
        if(!des){
            fprintf(stdout,"%#llx\n",scan);
        }
        if(typeid(*des) == typeid(RecordDescriptor)){
            uint64_t i = 0;
            for(const auto &f:static_cast<RecordDescriptor*>(des)->GetInfo()){
                if(f){
                    auto p = (uint64_t*)(scan+i);
                    if(*p==0){
                        continue;
                    }
                    *p = Forward(*p);
                }
                i+=WORD_SIZE;
            }
        }else{
            if(static_cast<ArrayDescriptor*>(des)->IsPointer()){
                for(uint64_t i = 0 ;i < des->GetSize();i+=WORD_SIZE){
                    auto p = (uint64_t*)(scan+i);
                    if(*p==0){
                        continue;
                    }
                    *p = Forward(*p);
                }
            }
        }
        scan += des->GetSize();
    }
    fprintf(stdout,"finish\n");
    decltype(addr2desc) updated_map;
    for(const auto&[from,to]:to_addr){
        updated_map.emplace(to,addr2desc[to]);
    }
    to_addr.clear();
    addr2desc = updated_map;

    auto temp = _from_space;
    _from_space = _to_space;
    _to_space = temp;
    fprintf(stdout,"swap,from:%#llx,to:%#llx\n",_from_space,_to_space);
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
    return (_heap_end-(uint64_t)_heap)/2 - from_offset;
}


void DerivedHeap::Initialize(uint64_t size){
    _heap = new char[size];
    _heap_end = (uint64_t)_heap + size;
    _from_space = (uint64_t)_heap;
    _to_space = (uint64_t)_heap + size/2;
    from_offset = 0;
    // fprintf(stdout,"init heap,from:%#llx,to:%#llx\n",_from_space,_to_space);
}

void DerivedHeap::GC(){
    // uint64_t *sp;
    // GET_TIGER_STACK(sp);
    // std::cout<<"sp:"<<sp<<std::endl;

    //tigerfunc -> alloc --> GC
    //cant use other functions to find roots
}

uint64_t DerivedHeap::Forward(uint64_t p){
    //point to from-space
    if(((uint64_t)_heap == _from_space && p < _to_space) || p >= _from_space){
        if(to_addr.count(p)){
            return to_addr[p];
        }
        fprintf(stdout,"finding:%#llx\n",p);
        auto size = addr2desc[p]->GetSize();
        for(auto i = 0; i < size ;i += WORD_SIZE){
            *(uint64_t*)(next+i) = *(uint64_t*)(p+i);
        }
        to_addr.emplace(p,next);
        addr2desc.emplace(next,addr2desc[p]);
        fprintf(stdout,"add addr:%#llx,size:%d\n",next,size);
        next += size;
        return to_addr[p];
    }else{
        fprintf(stdout,"heer\n");
        return p;
    }
}
} // namespace gc

