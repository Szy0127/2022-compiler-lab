#ifndef TIGER_PREPROCESS_PREPROCESSOR_H_
#define TIGER_PREPROCESS_PREPROCESSOR_H_

#include <fstream>

class Preprocessor{
public:
    Preprocessor() = delete;
    Preprocessor(std::string ifname,std::string ofname);
    void preprocess();
private:
    std::string ifname;
    std::string ofname;
};


#endif // TIGER_PREPROCESS_PREPROCESSOR_H_
