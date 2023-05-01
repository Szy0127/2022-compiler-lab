%filenames = "scanner"

 /*
  * Please don't modify the lines above.
  */

 /* You can add lex definitions here. */
 /* TODO: Put your lab2 code here */

%x COMMENT STRING IGNORE

%%


<COMMENT><<EOF>>  {errormsg_->Error(errormsg_->tok_pos_, "illegal comment: lost */");return 0;}
<STRING><<EOF>>   {errormsg_->Error(errormsg_->tok_pos_, "illegal string: lost \"");return 0;}
<IGNORE><<EOF>>   {errormsg_->Error(errormsg_->tok_pos_, "illegal string: lost \\");return 0;}
/*<INITIAL><<EOF>>  {if(number.empty())return 0;else{setMatched(number);number.clear();return Parser::INT;}}*/

<COMMENT>{
  "*/"   {adjust();comment_finish();}
  "/*"   {adjust();comment_begin();}
  \n      {adjust();errormsg_->Newline();}
  .      {adjust();}
}


<IGNORE>{
  \\      {ignore_finish();begin(StartCondition__::STRING);more();}
  [ \t]+  {more();}
  \n      {more();errormsg_->Newline();}
  .       {more();errormsg_->Error(char_pos_, "illegal string literal: only white space can be used between \\ ");}
}
<STRING>{
  \"                  {begin(StartCondition__::INITIAL);handle_string_finish();adjustStr();return Parser::STRING;}
  \\[tn"\\]           {handle_tn();more();}
  \\[[:digit:]]{3,3}    {handle_number();more();}
  \\\^[A-Z]           {handle_ctrl();more();}
  \\                  {ignore_begin();begin(StartCondition__::IGNORE);more();}
  .                   {more();}
}

<INITIAL>{

  "/*" {adjust();comment_begin();}
  \" {adjust();handle_string_begin();more();begin(StartCondition__::STRING);}


  "," {adjust(); return Parser::COMMA;}
  ":" {adjust(); return Parser::COLON;}
  ";" {adjust(); return Parser::SEMICOLON;}
  "." {adjust(); return Parser::DOT;}


  "(" {adjust(); return Parser::LPAREN;}
  ")" {adjust(); return Parser::RPAREN;}
  "[" {adjust(); return Parser::LBRACK;}
  "]" {adjust(); return Parser::RBRACK;}
  "{" {adjust(); return Parser::LBRACE;}
  "}" {adjust(); return Parser::RBRACE;}


  "+" {adjust(); return Parser::PLUS;}
  "+=" {adjust(); return Parser::PLUSS;}

  "-" {adjust(); return Parser::MINUS;}
  "-=" {adjust(); return Parser::MINUSS;}

  "*" {adjust(); return Parser::TIMES;}
  "*=" {adjust(); return Parser::TIMESS;}

  "/" {adjust(); return Parser::DIVIDE;}
  "/=" {adjust(); return Parser::DIVIDES;}


  "&" {adjust(); return Parser::AND;}
  "|" {adjust(); return Parser::OR;}

  "==" {adjust(); return Parser::EQ;}
  "!=" {adjust(); return Parser::NEQ;}
  "<" {adjust(); return Parser::LT;}
  "<=" {adjust(); return Parser::LE;}
  ">" {adjust(); return Parser::GT;}
  ">=" {adjust(); return Parser::GE;}

  "=" {adjust(); return Parser::ASSIGN;}

 /* reserved words */
  "nil" {adjust(); return Parser::NIL;}
  "var" {adjust(); return Parser::VAR;}
  "array" {adjust(); return Parser::ARRAY;}
  "if" {adjust(); return Parser::IF;}
  "then" {adjust(); return Parser::THEN;}
  "else" {adjust(); return Parser::ELSE;}
  "for" {adjust(); return Parser::FOR;}
  "to" {adjust(); return Parser::TO;}
  "while" {adjust(); return Parser::WHILE;}
  "do" {adjust(); return Parser::DO;}
  "break" {adjust(); return Parser::BREAK;}
  "let" {adjust(); return Parser::LET;}
  "in" {adjust(); return Parser::IN;}
  "end" {adjust(); return Parser::END;}
  "type" {adjust(); return Parser::TYPE;}
  "function" {adjust(); return Parser::FUNCTION;}
  "of" {adjust(); return Parser::OF;}

  "range" {adjust(); return Parser::RANGE;}
  "def" {adjust(); return Parser::DEF;}

  [[:alnum:]][[:alnum:]_]* {adjust();switch(dispose_id()){case 0:return Parser::ID;case 1:return Parser::INT;default:errormsg_->Error(errormsg_->tok_pos_, "illegal token");}}

  /*
  [[:alpha:]][[:alnum:]_]* {adjust(); if(int_flag){errormsg_->Error(errormsg_->tok_pos_, "illegal token");int_flag=false;}else{int_flag=false;return Parser::ID;}}
  */

  /*
  [[:alpha:]][[:alnum:]_]* {adjust();return Parser::ID;}
  [[:digit:]]+ {adjust();return Parser::INT;}

  */
  /*
  [[:digit:]]+[ \t]+ {adjust();return Parser::INT;}
  [[:digit:]]+\n {adjust();errormsg_->Newline();return Parser::INT;}
  [[:digit:]]+ {adjust();number=matched();}
  */

  /*
    * skip white space chars.
    * space, tabs and LF
    */
  [ \t]+ {adjust();}
  \n {adjust(); errormsg_->Newline();}

  /* illegal input */
  . {adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal token");}
}