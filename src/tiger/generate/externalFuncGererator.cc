#include "externalFuncGenerator.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <list>

static std::map<int,std::string> key2type = {
    {2,"long"},
    {3,"double"},
    {4,"struct string*"},
};

static std::map<std::string,std::string> type2print = {
    {"long","%ld"},
    {"double","%lf"},
    // {"string","printf(\"%s\\n\",%s);"},
};

ExternalFuncGenerator::ExternalFuncGenerator(std::string ofname,std::map<std::string,std::set<uint64_t>> external_functions)
:ofname(ofname),external_functions(external_functions){}

static std::list<int> key2list(uint64_t key){
    std::list<int> list;
    while(key){
        int k = key % 10;
        key /= 10;
        switch(k){
        case 2:
            list.push_front(2);
            break;
        case 3:
            list.push_front(3);
            break;
        case 4:
            list.push_front(4);
            break;
        default:
            // assert(false);
            break;
        }
    }
    return list;
}


void generate_print(std::ofstream &of,uint64_t key){
    auto list = key2list(key);
    of<<"extern \"C\" void print_"<<key<<"(";
    int i = 0;
    for(auto ki:list){
        if(i!=0){
            of<<",";
        }
        of<<key2type[ki]<<" a"<<i;
        i++;
    }
    of<<"){\n";
    i = 0;
    for(auto ki:list){
        if(i!=0){
            of<<"\tprintf(\" \");"<<std::endl;
        }
        if(ki == 4){//string
            of<<"\tprint_string(a"<<i<<");"<<std::endl;
        }else{
            of<<"\tprintf(\""<<type2print[key2type[ki]]<<"\",a"<<i<<");"<<std::endl;
        }
        i++;
    }
    of<<"\tprintf(\"\\n\");"<<std::endl;
    of<<"}\n";
}

void ExternalFuncGenerator::generate(){
    std::ofstream out(ofname);
    out<<"#include <stdio.h>\n";

    //meet problems in link if write print_string in runtime.cc
    out<<"struct string { \n\
        int length;         \n\
        unsigned char chars[1]; \n\
    };                              \n\
    void print_string(struct string *s) {   \n\
        int i;                               \n\       
        unsigned char *p = s->chars;            \n\
        for (i = 0; i < s->length; i++, p++) putchar(*p);  \n\
    }\n";

    for(auto [k,v]:external_functions){
      if(k == "print"){
        for(auto key:v){
            generate_print(out,key);
        }
      }
    }

}