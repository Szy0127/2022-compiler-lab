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
  ASSIGN
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF
  BREAK NIL
  FUNCTION VAR TYPE
  
  RANGE
  PLUSS MINUSS TIMESS DIVIDES MODS
  DEF
  RETURN

  TRUE FALSE

 /* token priority */

%left OR 
%left AND
%nonassoc GE GT LE LT EQ NEQ
%left MINUS PLUS
%left DIVIDE TIMES
%left MOD
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

lvalue:  lvalue DOT ID {$$ = new absyn::FieldVar(scanner_.GetTokPos(), $1,$3);}
  | lvalue LBRACK exp RBRACK {$$ = new absyn::SubscriptVar(scanner_.GetTokPos(), $1,$3);}
  | ID  {$$ = new absyn::SimpleVar(scanner_.GetTokPos(), $1);}
  | one {$$ = $1;}
  ;


/* shift/reduce conflict in a[10] and a[10] of 1 */
one : ID LBRACK exp RBRACK {$$ = new absyn::SubscriptVar(scanner_.GetTokPos(), new absyn::SimpleVar(scanner_.GetTokPos(), $1),$3);};

 /* TODO: Put your lab3 code here */

exp : LET decs_nonempty IN sequencing_exps END {$$ = new absyn::LetExp(scanner_.GetTokPos(),$2,new absyn::SeqExp(scanner_.GetTokPos(),$4));}
  /*| ID LBRACK exp RBRACK OF exp {$$ = new absyn::ArrayExp(scanner_.GetTokPos(),$1,$3,$6);}*/
  | one OF exp {auto scvar = static_cast<absyn::SubscriptVar*>($1);
    auto svar = static_cast<absyn::SimpleVar*>(scvar->var_);
    $$ = new absyn::ArrayExp(scanner_.GetTokPos(),svar->sym_,scvar->subscript_,$3);}
    /*fake subscriptvar is unused*/
  | one {$$ = new absyn::VarExp(scanner_.GetTokPos(),$1);}
  | INT {$$ = new absyn::IntExp(scanner_.GetTokPos(),$1);}

  | TRUE {$$ = new absyn::IntExp(scanner_.GetTokPos(),1);}
  | FALSE {$$ = new absyn::IntExp(scanner_.GetTokPos(),0);}

  | lvalue {$$ = new absyn::VarExp(scanner_.GetTokPos(),$1);}
  | ID LBRACE rec RBRACE {$$ = new absyn::RecordExp(scanner_.GetTokPos(),$1,$3);}
  | STRING {$$ = new absyn::StringExp(scanner_.GetTokPos(), $1);}
  | lvalue ASSIGN exp {$$ = new absyn::AssignExp(scanner_.GetTokPos(),$1,$3);}

  | lvalue PLUSS exp {$$ = new absyn::AssignExp(scanner_.GetTokPos(),$1,new absyn::OpExp(scanner_.GetTokPos(),absyn::PLUS_OP,new absyn::VarExp(scanner_.GetTokPos(),$1),$3));}
  | lvalue MINUSS exp {$$ = new absyn::AssignExp(scanner_.GetTokPos(),$1,new absyn::OpExp(scanner_.GetTokPos(),absyn::MINUS_OP,new absyn::VarExp(scanner_.GetTokPos(),$1),$3));}
  | lvalue TIMESS exp {$$ = new absyn::AssignExp(scanner_.GetTokPos(),$1,new absyn::OpExp(scanner_.GetTokPos(),absyn::TIMES_OP,new absyn::VarExp(scanner_.GetTokPos(),$1),$3));}
  | lvalue DIVIDES exp {$$ = new absyn::AssignExp(scanner_.GetTokPos(),$1,new absyn::OpExp(scanner_.GetTokPos(),absyn::DIVIDE_OP,new absyn::VarExp(scanner_.GetTokPos(),$1),$3));}
  | lvalue MODS exp {$$ = new absyn::AssignExp(scanner_.GetTokPos(),$1,new absyn::OpExp(scanner_.GetTokPos(),absyn::MOD_OP,new absyn::VarExp(scanner_.GetTokPos(),$1),$3));}



  | IF exp COLON exp ELSE COLON exp {$$ = new absyn::IfExp(scanner_.GetTokPos(),$2,$4,$7);}
  | IF exp COLON exp {$$ = new absyn::IfExp(scanner_.GetTokPos(),$2,$4,nullptr);}

  | ID LPAREN RPAREN {$$ = new absyn::CallExp(scanner_.GetTokPos(),$1,new absyn::ExpList());}
  | ID LPAREN actuals RPAREN {$$ = new absyn::CallExp(scanner_.GetTokPos(),$1,$3);}
  | exp EQ exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::EQ_OP,$1,$3);}
  | exp NEQ exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::NEQ_OP,$1,$3);}
  | exp TIMES exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::TIMES_OP,$1,$3);}
  | exp DIVIDE exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::DIVIDE_OP,$1,$3);}
  | exp MINUS exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::MINUS_OP,$1,$3);}
  | exp PLUS exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::PLUS_OP,$1,$3);}
  | exp MOD exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::MOD_OP,$1,$3);}

  | exp GT exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::GT_OP,$1,$3);}
  | exp LT exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::LT_OP,$1,$3);}
  | exp LE exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::LE_OP,$1,$3);}
  | exp GE exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::GE_OP,$1,$3);}
  | exp AND exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::AND_OP,$1,$3);}
  | exp OR exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::OR_OP,$1,$3);}
  | MINUS exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::MINUS_OP,new absyn::IntExp(scanner_.GetTokPos(),0),$2); }
  | NIL {$$ = new absyn::NilExp(scanner_.GetTokPos());}
  | LPAREN exp RPAREN {$$ = $2;}
  | LPAREN sequencing_exps RPAREN {$$ = new absyn::SeqExp(scanner_.GetTokPos(),$2);}
  | WHILE exp DO exp {$$ = new absyn::WhileExp(scanner_.GetTokPos(),$2,$4);}
  | FOR ID ASSIGN exp TO exp DO exp {$$ = new absyn::ForExp(scanner_.GetTokPos(),$2,$4,$6,$8);}

  | FOR ID IN RANGE LPAREN exp COMMA exp RPAREN COLON exp {$$ = new absyn::ForExp(scanner_.GetTokPos(),$2,$6,$8,$11);}
  | FOR ID IN RANGE LPAREN exp RPAREN COLON exp {$$ = new absyn::ForExp(scanner_.GetTokPos(),$2,new absyn::IntExp(scanner_.GetTokPos(),0),$6,$9);}

  | BREAK {$$ = new absyn::BreakExp(scanner_.GetTokPos());}
  | RETURN exp {$$ = new absyn::ReturnExp(scanner_.GetTokPos(),$2);}
  | {$$ = new absyn::VoidExp(scanner_.GetTokPos());}
  ;

nonemptyactuals : exp COMMA actuals  {$$=$3->Prepend($1);}
  | exp {$$ = new absyn::ExpList($1);}
  ;
actuals : exp COMMA actuals  {$$=$3->Prepend($1);}
  | exp {$$ = new absyn::ExpList($1);}
  | {$$ = new absyn::ExpList();}
  ;

sequencing_exps : exp SEMICOLON sequencing {$$=$3->Prepend($1);}
  | exp {$$ = new absyn::ExpList($1);}
  ;
sequencing : exp SEMICOLON sequencing {$$=$3->Prepend($1);}
  | exp {$$ = new absyn::ExpList($1);}
  | {$$ = new absyn::ExpList();}
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
fundec_one : DEF ID LPAREN tyfields RPAREN COLON exp  {$$ = new absyn::FunDec(scanner_.GetTokPos(),$2,$4,nullptr,$7);}
  | DEF ID LPAREN tyfields RPAREN COLON ID COLON exp {$$ = new absyn::FunDec(scanner_.GetTokPos(),$2,$4,$7,$9);}
  ;



ty : ID {$$ = new absyn::NameTy(scanner_.GetTokPos(),$1);}
  | LBRACE tyfields_nonempty RBRACE {$$ = new absyn::RecordTy(scanner_.GetTokPos(),$2);}
  | ARRAY OF ID {$$ = new absyn::ArrayTy(scanner_.GetTokPos(),$3);}
  ;

// tyfields_nonempty : tyfield COMMA tyfields {$$=$3->Prepend($1);}
//   | tyfield {$$ = new absyn::FieldList($1);}
//   ;
// tyfields : tyfield COMMA tyfields {$$=$3->Prepend($1);}
//   | tyfield {$$ = new absyn::FieldList($1);}
//   | {$$ = new absyn::FieldList();}
//   ;

// tyfield : ID COLON ID {$$ = new absyn::Field(scanner_.GetTokPos(),$1,$3);};


tyfields : tyfield COMMA tyfields {$$=$3->Prepend($1);}
  | tyfield {$$ = new absyn::FieldList($1);}
  | {$$ = new absyn::FieldList();}
  ;

tyfield : ID {$$ = new absyn::Field(scanner_.GetTokPos(),$1,nullptr);};

rec_nonempty : rec_one COMMA rec {if($3)$$=$3->Prepend($1);else $$ = new absyn::EFieldList($1);}
  | rec_one {$$ = new absyn::EFieldList($1);}
  ;

rec : rec_one COMMA rec {$$=$3->Prepend($1);}
  | rec_one {$$ = new absyn::EFieldList($1);}
  | {$$ = new absyn::EFieldList();}
  ;


rec_one : ID EQ exp {$$ = new absyn::EField($1,$3);};