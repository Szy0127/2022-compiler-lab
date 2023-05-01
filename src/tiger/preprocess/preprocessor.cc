#include "preprocessor.h"
#include <fstream>
#include <iostream>
#include <stack>
#include <tuple>
Preprocessor::Preprocessor(std::string ifname,std::string ofname):ifname(ifname),ofname(ofname){}

std::tuple<std::string,bool> processFunc(std::string defexp){
    std::string result;
    int index = 0;
    char c = defexp[index];
    while(c == '\t' || c== ' '){
        c = defexp[++index];
    }
    if(defexp.substr(index,3) != "def"){
        return std::tuple<std::string,bool>("",false);
    }
    while(c != ':'){
        result.push_back(c);
        c = defexp[++index];
    }
    result.push_back('=');
    return std::tuple<std::string,bool>(defexp,true);
}

enum DefStatus{
    before_let,
    after_let,
    before_in,
    after_in,
    before_end
};

void Preprocessor::preprocess(){
    std::ifstream input(ifname);
    std::ofstream output(ofname);
    std::string buf;
    std::getline(input,buf);
    output<<"/*preprocessed*/"<<std::endl;
    bool first = true;
    std::stack<int> indentation;
    std::stack<std::pair<DefStatus,int>> def_record;
    def_record.emplace(before_let,0);
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
                    if(cur_indent == def_record.top().second && def_record.top().first == after_let){
                            def_record.pop();
                            def_record.emplace(before_in,cur_indent);
                    }
                    if(cur_indent < def_record.top().second &&  def_record.top().first == after_in){
                            def_record.pop();
                            def_record.emplace(before_end,cur_indent);
                    }
                }
            }
        }
        std::tuple<std::string,int> ret = processFunc(buf);
        if(std::get<1>(ret)){
            if(def_record.top().first == before_let){
                output<<"let"<<std::endl;
            }else{
                def_record.pop();
            }
            def_record.emplace(after_let,cur_indent);
            buf = std::get<0>(ret);
        }else{
            auto indent = def_record.top().second;
            if(def_record.top().first == before_in){
                output<<"in("<<std::endl;
                def_record.pop();
                def_record.emplace(after_in,indent);
            }else if(def_record.top().first == before_end){
                output<<")end"<<std::endl;
                def_record.pop();
            }else{
                if(first){
                    output<<"("<<std::endl;
                }else{
                    output<<";"<<std::endl;
                }
            }
        }
        output<<buf;
        std::getline(input,buf);
    }
    output<<")"<<std::endl;
    if(def_record.top().first >=after_in){
        output<<"end"<<std::endl;
    }
}