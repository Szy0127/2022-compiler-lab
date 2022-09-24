%filenames = "scanner"

 /*
  * Please don't modify the lines above.
  */

 /* You can add lex definitions here. */
 /* TODO: Put your lab2 code here */

%x COMMENT STR IGNORE

%%


 /* reserved words */
"array" {adjust(); return Parser::ARRAY;}
 /* TODO: Put your lab2 code here */

<COMMENT>{
  "*/"   {adjust();begin(StartCondition__::INITIAL);}
  \n      {adjust();}
  .      {adjust();}
}


<IGNORE>{

  \\      {ignore_finish();begin(StartCondition__::STR);more();}
  [ \t]+  {more();}
  \n      {more(); }/*errormsg_->Newline();}*/
  /*.       {more();}*/
}
<STR>{
  \"                  {begin(StartCondition__::INITIAL);handle_string_finish();adjustStr();return Parser::STRING;}
  \\[tn"\\]           {handle_tn();more();}
  \\[0-9][0-9][0-9]   {handle_ddd();more();}
  \\\^[A-Z]           {handle_ctrl();more();}
  \\                  {ignore_begin();begin(StartCondition__::IGNORE);more();}
  .                   {more();}
}

<INITIAL>{

  "/*" {adjust();begin(StartCondition__::COMMENT);}
  \" {adjust();handle_string_begin();more();begin(StartCondition__::STR);}


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
  "-" {adjust(); return Parser::MINUS;}
  "*" {adjust(); return Parser::TIMES;}
  "/" {adjust(); return Parser::DIVIDE;}

  "&" {adjust(); return Parser::AND;}
  "|" {adjust(); return Parser::OR;}

  "=" {adjust(); return Parser::EQ;}
  "<>" {adjust(); return Parser::NEQ;}
  "<" {adjust(); return Parser::LT;}
  "<=" {adjust(); return Parser::LE;}
  ">" {adjust(); return Parser::GT;}
  ">=" {adjust(); return Parser::GE;}



  ":=" {adjust(); return Parser::ASSIGN;}
  "nil" {adjust(); return Parser::NIL;}
  "var" {adjust(); return Parser::VAR;}

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



  [[:alpha:]][[:alnum:]_]* {adjust(); return Parser::ID;}
  [[:digit:]]+ {adjust(); return Parser::INT;}


  /*
    * skip white space chars.
    * space, tabs and LF
    */
  [ \t]+ {adjust();}
  \n {adjust(); errormsg_->Newline();}

  /* illegal input */
  . {adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal token");}
}