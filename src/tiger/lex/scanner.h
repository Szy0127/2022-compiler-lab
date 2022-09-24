#ifndef TIGER_LEX_SCANNER_H_
#define TIGER_LEX_SCANNER_H_

#include <algorithm>
#include <cstdarg>
#include <iostream>
#include <string>

#include "scannerbase.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/parse/parserbase.h"

class Scanner : public ScannerBase {
public:
  Scanner() = delete;
  explicit Scanner(std::string_view fname, std::ostream &out = std::cout)
      : ScannerBase(std::cin, out), comment_level_(1), char_pos_(1),
        errormsg_(std::make_unique<err::ErrorMsg>(fname)) {
    switchStreams(errormsg_->infile_, out);
  }

  /**
   * Output an error
   * @param message error message
   */
  void Error(int pos, std::string message, ...) {
    va_list ap;
    va_start(ap, message);
    errormsg_->Error(pos, message, ap);
    va_end(ap);
  }

  /**
   * Getter for `tok_pos_`
   */
  [[nodiscard]] int GetTokPos() const { return errormsg_->GetTokPos(); }

  /**
   * Transfer the ownership of `errormsg_` to the outer scope
   * @return unique pointer to errormsg
   */
  [[nodiscard]] std::unique_ptr<err::ErrorMsg> TransferErrormsg() {
    return std::move(errormsg_);
  }

  int lex();

private:
  int comment_level_;
  std::string string_buf_;
  int char_pos_;
  std::unique_ptr<err::ErrorMsg> errormsg_;

  // std::string string_before_ignore;
  // bool ignore_newline_flag = false;
  size_t ignore_begin_length;

  /**
   * NOTE: do not change all the funtion signature below, which is used by
   * flexc++ internally
   */
  int lex__();
  int executeAction__(size_t ruleNr);

  void print();
  void preCode();
  void postCode(PostEnum__ type);
  void adjust();
  void adjustStr();

  void handle_string_begin();
  void handle_string_finish();
  void handle_tn();
  void handle_ddd();
  void handle_ctrl();

  // void ignore();
  // void ignore_newline();
  void ignore_begin();
  void ignore_finish();
};

inline int Scanner::lex() { return lex__(); }

inline void Scanner::preCode() {
  // Optionally replace by your own code
}

inline void Scanner::postCode(PostEnum__ type) {
  // Optionally replace by your own code
}

inline void Scanner::print() { print__(); }

inline void Scanner::adjust() {
  errormsg_->tok_pos_ = char_pos_;
  char_pos_ += length();
}

inline void Scanner::adjustStr() { char_pos_ += length(); }

inline void Scanner::handle_string_finish(){
  std::string s = matched();
  // s.erase(0,1);
  s.pop_back();
  setMatched(s);
  char_pos_ += 1;
  // std::cout<<"string:"<<s<<std::endl;
}
inline void Scanner::handle_string_begin(){
  setMatched("");
  // char_pos_ += 1;
  // errormsg_->tok_pos_++;
  // according to answer, pos of string is the location after "
}


inline void Scanner::handle_tn(){
  std::string s = matched();
  char c = s[s.length()-1];
  // std::cout<<c<<std::endl;
  s.erase(s.end()-2,s.end());
  switch(c){
    case 'n':s.push_back('\n');break;
    case 't':s.push_back('\t');break;
    case '"':s.push_back('"');break;
    case '\\':s.push_back('\\');break;
  }
  // std::cout<<s<<std::endl;
  setMatched(s);
  char_pos_ += 1;
}
inline void Scanner::handle_ctrl(){
  std::string s = matched();
  char c = s[s.length()-1];
  // std::cout<<c<<std::endl;
  s.erase(s.end()-3,s.end());
  s.push_back(c-'A'+1);
  // std::cout<<s<<std::endl;
  setMatched(s);
  char_pos_ += 2;
}


inline void Scanner::handle_ddd(){
  std::string s = matched();
  std::string code = s.substr(s.length()-3);
  s.erase(s.end()-4,s.end());
  char c = std::stoi(code);
  // std::cout<<c<<std::endl;
  s.push_back(c);
  setMatched(s);

  char_pos_ += 3;

}


inline void Scanner::ignore_begin(){
  ignore_begin_length = matched().length();
  // std::cout<<ignore_begin_length<<std::endl;
}

inline void Scanner::ignore_finish(){
  std::string s = matched();
  char_pos_ += s.length()-ignore_begin_length+1;
  s.erase(s.begin()+ignore_begin_length-1,s.end());
  setMatched(s);
}
// inline void Scanner::ignore(){
//   if(ignore_newline_flag){
//     setMatched(string_before_ignore);
//     std::cout<<"reset:"<<string_before_ignore<<std::endl;
//   }else{
//       std::string s = matched();
//       std::cout<<"before ignore:"<<s<<"#"<<std::endl;
//       s.pop_back();
//       std::cout<<"after ignore:"<<s<<"#"<<std::endl;
//       setMatched(s);
//       string_before_ignore = s;
//   }
//   ignore_newline_flag = false;

// }

// inline void Scanner::ignore_newline(){
//   std::string s = matched();
//   std::cout<<"ignore newline:"<<s<<std::endl;
//   s.pop_back();
//   setMatched(s);
//   string_before_ignore = s;
//   ignore_newline_flag = true;
// }


#endif // TIGER_LEX_SCANNER_H_
