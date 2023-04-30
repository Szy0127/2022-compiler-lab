#include "preprocessor.h"
#include <fstream>
Preprocessor::Preprocessor(std::string ifname,std::string ofname):ifname(ifname),ofname(ofname){}

void Preprocessor::preprocess(){
    std::ifstream input(ifname);
    std::ofstream output(ofname);
    std::string buf;
    std::getline(input,buf);
    output<<"/*preprocessed*/"<<std::endl;
    while(!input.eof()){
        output<<buf<<std::endl;
        std::getline(input,buf);
    }
}