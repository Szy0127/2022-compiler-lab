%filenames parser
%scanner tiger/lex/scanner.h
%baseclass-preinclude tiger/absyn/absyn.h

 /*
  * Please don't modify the lines above.
  */

%union {
  int ival;
  std::string* sval;
  sym::Symbol *sym;
  absyn::Exp *exp;
  absyn::ExpList *explist;
  absyn::Var *var;
  absyn::DecList *declist;
  absyn::Dec *dec;
  absyn::EFieldList *efieldlist;
  absyn::EField *efield;
  absyn::NameAndTyList *tydeclist;
  absyn::NameAndTy *tydec;
  absyn::FieldList *fieldlist;
  absyn::Field *field;
  absyn::FunDecList *fundeclist;
  absyn::FunDec *fundec;
  absyn::Ty *ty;
  }

%token <sym> ID
%token <sval> STRING
%token <ival> INT

%token
  COMMA COLON SEMICOLON LPAREN RPAREN LBRACK RBRACK
  LBRACE RBRACE DOT
  PLUS MINUS TIMES DIVIDE EQ NEQ LT LE GT GE
  AND OR ASSIGN
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF
  BREAK NIL
  FUNCTION VAR TYPE

 /* token priority */
 /* TODO: Put your lab3 code here */

%type <exp> exp expseq
%type <explist> actuals nonemptyactuals sequencing sequencing_exps
%type <var> lvalue one oneormore
%type <declist> decs decs_nonempty
%type <dec> decs_nonempty_s vardec
%type <efieldlist> rec rec_nonempty
%type <efield> rec_one
%type <tydeclist> tydec
%type <tydec> tydec_one
%type <fieldlist> tyfields tyfields_nonempty
%type <field> tyfield
%type <ty> ty
%type <fundeclist> fundec
%type <fundec> fundec_one

%start program

%%
program:  exp  {absyn_tree_ = std::make_unique<absyn::AbsynTree>($1);};

lvalue:  ID  {$$ = new absyn::SimpleVar(scanner_.GetTokPos(), $1);}
  |  oneormore  {$$ = $1;}
  ;

 /* TODO: Put your lab3 code here */

exp : LET decs_nonempty IN sequencing_exps END {$$ = new absyn::LetExp(scanner_.GetTokPos(),$2,new absyn::SeqExp(scanner_.GetTokPos(),$4));}
  | ID LBRACK exp RBRACK OF exp {$$ = new absyn::ArrayExp(scanner_.GetTokPos(),$1,$3,$6);}
  | INT {$$ = new absyn::IntExp(scanner_.GetTokPos(),$1);}
  | lvalue {$$ = new absyn::VarExp(scanner_.GetTokPos(),$1);}
  ;


sequencing_exps : exp sequencing {if($2)$$=$2->Prepend($1);else $$ = new absyn::ExpList($1);};
sequencing : exp sequencing {if($2)$$=$2->Prepend($1);else $$ = new absyn::ExpList($1);}
  |
  ;

decs_nonempty : decs_nonempty_s decs {if($2)$$=$2->Prepend($1);else $$ = new absyn::DecList($1);};
decs : decs_nonempty_s decs {if($2)$$=$2->Prepend($1);else $$ = new absyn::DecList($1);}
  | 
  ;


decs_nonempty_s : tydec {$$ = new absyn::TypeDec(scanner_.GetTokPos(),$1);}
  | vardec {$$ = $1;}
  | fundec {$$ = new absyn::FunctionDec(scanner_.GetTokPos(),$1);}
  ;

vardec : VAR ID ASSIGN exp {$$ = new absyn::VarDec(scanner_.GetTokPos(),$2,nullptr,$4);}
  | VAR ID COLON ID ASSIGN exp {$$ = new absyn::VarDec(scanner_.GetTokPos(),$2,$4,$6);}
  ;

tydec : tydec_one tydec {if($2)$$=$2->Prepend($1);else $$ = new absyn::NameAndTyList($1);}
  | 
  ;
tydec_one : TYPE ID EQ ty {$$ = new absyn::NameAndTy($2,$4);};

fundec : fundec_one fundec {if($2)$$=$2->Prepend($1);else $$ = new absyn::FunDecList($1);}
  | 
  ;
fundec_one : FUNCTION ID LPAREN tyfields RPAREN EQ exp {$$ = new absyn::FunDec(scanner_.GetTokPos(),$2,$4,nullptr,$7);}
  | FUNCTION ID LPAREN tyfields RPAREN COLON ID EQ exp {$$ = new absyn::FunDec(scanner_.GetTokPos(),$2,$4,$7,$9);}
  ;



ty : ID {$$ = new absyn::NameTy(scanner_.GetTokPos(),$1);}
  | LBRACE tyfields_nonempty RBRACE {$$ = new absyn::RecordTy(scanner_.GetTokPos(),$2);}
  | ARRAY OF ID {$$ = new absyn::ArrayTy(scanner_.GetTokPos(),$3);}
  ;

tyfields_nonempty : tyfield COMMA tyfields {if($3)$$=$3->Prepend($1);else $$ = new absyn::FieldList($1);}
  | tyfield {$$ = new absyn::FieldList($1);}
  ;
tyfields : tyfield COMMA tyfields {if($3)$$=$3->Prepend($1);else $$ = new absyn::FieldList($1);}
  | tyfield {$$ = new absyn::FieldList($1);}
  |
  ;

tyfield : ID COLON ID {$$ = new absyn::Field(scanner_.GetTokPos(),$1,$3);};


