#include "preprocessor.h"
#include <fstream>
#include <iostream>
#include <stack>
#include <tuple>
Preprocessor::Preprocessor(std::string ifname,std::string ofname):ifname(ifname),ofname(ofname){}


void Preprocessor::preprocess(){
    std::ifstream input(ifname);
    std::ofstream output(ofname);
    std::string buf;
    std::getline(input,buf);
    output<<"/*preprocessed*/"<<std::endl;
    // output<<"("<<std::endl;

    bool first = true;
    std::stack<int> indentation;
    while(!input.eof()){
        if(buf[0] == '\n'){
            std::getline(input,buf);
            continue;
        }
        int cur_indent = 0;
        while(buf[cur_indent]=='\t'){
            cur_indent++;
        }
        // // std::cout<<"buf:#"<<buf<<"#"<<cur_indent<<std::endl;
        if(indentation.empty() ||  cur_indent>indentation.top()){
            indentation.push(cur_indent);
            output<<"("<<std::endl;
            first = true;
        }else{
            first = false;
            while(cur_indent < indentation.top()){
                indentation.pop();
                output<<")"<<std::endl;
            }
        }
        if(!first){
            output<<";"<<std::endl;
        }
        output<<buf;
        std::getline(input,buf);
    }
    output<<")"<<std::endl;
    // if(def_record.top().first >=after_in){
    //     output<<"end"<<std::endl;
    // }
}