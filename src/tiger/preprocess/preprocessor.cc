#include "preprocessor.h"
#include <fstream>
#include <iostream>
#include <stack>
Preprocessor::Preprocessor(std::string ifname,std::string ofname):ifname(ifname),ofname(ofname){}

std::string processFor(std::string forexp){
    std::string result;
    int index = 0;
    char c = forexp[index];
    while(c == '\t' || c== ' '){
        c = forexp[++index];
    }
    if(forexp.substr(index,3) != "for"){
        return forexp;
    }
    while(c != ':'){
        result.push_back(c);
        c = forexp[++index];
    }
    result += "do";
    return result;
}

void Preprocessor::preprocess(){
    std::ifstream input(ifname);
    std::ofstream output(ofname);
    std::string buf;
    std::getline(input,buf);
    output<<"/*preprocessed*/"<<std::endl;
    bool first = true;
    std::stack<int> indentation;
    while(!input.eof()){
        int cur_indent = 0;
        while(buf[cur_indent]=='\t'){
            cur_indent++;
        }
        // std::cout<<"buf:#"<<buf<<"#"<<cur_indent<<std::endl;
        if(indentation.empty() ||  cur_indent>indentation.top()){
            indentation.push(cur_indent);
            first = true;
        }else{
            first = false;
            if(cur_indent < indentation.top()){
                indentation.pop();
                if(cur_indent != indentation.top()){
                    std::cerr<<"indentation error"<<std::endl;
                    exit(1);
                }else{
                    output<<")"<<std::endl;
                }
            }
        }
        if(first){
            output<<"("<<std::endl;
        }else{
            output<<";"<<std::endl;
        }
        // buf = processFor(buf);
        output<<buf;
        std::getline(input,buf);
    }
    output<<")"<<std::endl;
}