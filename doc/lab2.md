# Lab 2: Lexical Analysis

## useful things in manual

[Flexc++ (Version 2.07.00) User Guide (fbb-git.github.io)](https://fbb-git.github.io/flexcpp/manual/flexc++.html)

- 2.5: Members and macros

- 3.4: Patterns

- 3.6.1: Notation details

## handle comments

all characters between /* and /* should be ignored, so a stated COMMENT should be used

```
//tiger.lex
<INITIAL>"/*" {adjust();comment_begin();}
<COMMENT>"*/"   {adjust();comment_finish();}
```

we can just do nothing in COMMENT state when encounting any character except */ ,but pay attention that . can not match \n

```
//tiger.lex
<COMMENT>\n     {adjust();errormsg_->Newline();}
<COMMENT>.      {adjust();}
```

comments are allowed to be nested, so we should guatantee matching of each /* */ pair

Instead of digging into the advance usage of flexc++, I choose to implement by myself in scanner.h

```c++
//scanner.h
int comment_depth = 0;
void Scanner::comment_begin() {
  if (comment_depth == 0) {
    begin(StartCondition__::COMMENT);
  }
  comment_depth++;
}
void Scanner::comment_finish() {
  comment_depth--;
  if (comment_depth == 0) {
    begin(StartCondition__::INITIAL);
  }
}
```

besides,we should do something when a comment begins in a comment

```
  //tiger.lex
 <COMMENT>"/*"   {adjust();comment_begin();}
```

- \*/ before /\*: illegal token
- no \*/ after /\*: Error defined by myself

## handle strings

all characters between "  " should not be recognized as any token, so a stated STRING should be used

```
//tiger.lex
<INITIAL>\" {adjust();handle_string_begin();more();begin(StartCondition__::STRING);}
<STRING>\"            {begin(StartCondition__::INITIAL);handle_string_finish();adjustStr();return Parser::STRING;}
```

we should modify the string a little,such as remove "", substitute character instead of numbers with certain format etc. In order to implement that, we can write a callback function in C++, dispose the entire string which lex catch for us. But it's a little complicated to deal with string in C++, we don't have api to simply replace all remove words. At last, I choose to use lex to match certain format and replace them each time.

example:

```
//tiger.lex
<STRING>\\[[:digit:]]{3,3}    {handle_ddd();more();}
```

 ```c++
 //scanner.h
 void Scanner::handle_number() {
   std::string s = matched();
   std::string code = s.substr(s.length() - 3);
   int ci = std::stoi(code);
   if(ci >= 128){
     errormsg_->Error(char_pos_, "illegal string literal: /ddd 0-127");
     return;
   }
   char c = static_cast<char>(ci);
   s.erase(s.end() - 4, s.end());
   s.push_back(c);
   setMatched(s);
   char_pos_ += 3;
 }
 ```

use **matched** and **setMatched** to substitute the string and calculate the difference of position. 

In IGNORE,  I choose to read any white space(\t\n etc.) and do nothing. After IGNORE, just discard them together.

```
//tiger.lex
<STRING>\\ {ignore_begin();begin(StartCondition__::IGNORE);more();}
<IGNORE>{
  \\      {ignore_finish();begin(StartCondition__::STRING);more();}
  [ \t]+  {more();}
  \n      {more();errormsg_->Newline();}
  .       {more();errormsg_->Error(char_pos_, "illegal string literal: only white space can be used between / ");}
}
```

```c++
//scanner.h
size_t ignore_begin_length = 0;
void Scanner::ignore_begin() {
  ignore_begin_length = matched().length();
}
void Scanner::ignore_finish() {
  std::string s = matched();
  char_pos_ += s.length() - ignore_begin_length + 1;
  s.erase(s.begin() + ignore_begin_length - 1, s.end());
  setMatched(s);
}
```

## error handling

- \ddd not belongs to 0-127:scanner.h
- \f___f\ contains character which is not white space:scanner.h
- dismatch of " and /*: EOF in tiger.lex
- wrong token: tiger.lex

## end-of-file handling

```
//tiger.lex
<COMMENT><<EOF>> {errormsg_->Error(tok_pos_, "illegal comment: lost */");}
<STRING><<EOF>> {errormsg_->Error(tok_pos_, "illegal string: lost \"");}
<IGNORE><<EOF>> {errormsg_->Error(tok_pos_, "illegal string: lost \"");}
```