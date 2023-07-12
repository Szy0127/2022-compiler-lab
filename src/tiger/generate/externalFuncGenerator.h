#ifndef TIGER_GENERATE_EXTERNALFUNCGENERATOR_H_
#define TIGER_GENERATE_EXTERNALFUNCGENERATOR_H_

#include <fstream>
#include <map>
#include <set>

class ExternalFuncGenerator{
public:
    ExternalFuncGenerator() = delete;
    ExternalFuncGenerator(std::string ofname,std::map<std::string,std::set<uint64_t>> external_functions);
    void generate();
private:
    std::string ofname;
    std::map<std::string,std::set<uint64_t>> external_functions;

};


#endif // TIGER_GENERATE_EXTERNALFUNCGENERATOR_H_
