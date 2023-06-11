#include "preprocessor.h"
#include <fstream>
#include <iostream>
#include <stack>
#include <tuple>

Preprocessor::Preprocessor(std::string ifname,std::string ofname):ifname(ifname),ofname(ofname){}

std::tuple<int,int> get_indent(std::string line){
    int cur_indent = 0;
    int index = 0;
    auto len = line.length();
    while(index < len && line[index++]=='\t'){
        cur_indent++;
    }
    index--;
    int space_count = 0;
    while(index < len && line[index++]==' '){
        space_count++;
        if(space_count == 4){
            space_count = 0;
            cur_indent++;
        }
    }
    index--;
    //error if not multiple of 4
    return {cur_indent,index};
}

bool empty_line(std::string line){
    for(auto c:line){
        if(c!=' ' && c!='\t' && c != '\n'){
            return false;
        }
    }
    return true;
}

void Preprocessor::preprocess(){
    std::ifstream input(ifname);
    std::ofstream output(ofname);
    std::string buf;
    std::getline(input,buf);
    output<<"/*preprocessed*/"<<std::endl;
    // output<<"("<<std::endl;

    bool first = true;
    std::stack<int> indentation;
    while(!input.eof() || !buf.empty()){
        bool empty = empty_line(buf);
        auto [cur_indent,index] = get_indent(buf);
        if(empty){
            cur_indent = 0;
        }
        // std::cout<<"buf:"<<buf.length()<<"#"<<int(buf[0])<<buf<<"#"<<cur_indent<<std::endl;
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
        if(!empty && buf[index] != '#'){
            if(!first){
                output<<";"<<std::endl;
            }
            output<<buf;
        }

        if(input.eof()){
            break;
        }
        std::getline(input,buf);
    }
    while(!indentation.empty()){
        indentation.pop();
        output<<")"<<std::endl;
    }
}