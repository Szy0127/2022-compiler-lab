#include "preprocessor.h"
#include <fstream>
Preprocessor::Preprocessor(std::string ifname,std::string ofname):ifname(ifname),ofname(ofname){}

void Preprocessor::preprocess(){
    std::ifstream input(ifname);
    std::ofstream output(ofname);
    std::string buf;
    std::getline(input,buf);
    output<<"/*preprocessed*/"<<std::endl;
    output<<"("<<std::endl;
    bool first = true;
    while(!input.eof()){
        if(!first){
            output<<";"<<std::endl;
        }
        first = false;
        output<<buf;
        std::getline(input,buf);
    }
    output<<")"<<std::endl;
}