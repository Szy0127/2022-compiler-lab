#include "tiger/absyn/absyn.h"
#include "tiger/escape/escape.h"
#include "tiger/frame/x64frame.h"
#include "tiger/output/logger.h"
#include "tiger/output/output.h"
#include "tiger/parse/parser.h"
#include "tiger/translate/translate.h"
#include "tiger/semant/semant.h"
#include "tiger/preprocess/preprocessor.h"
#include "tiger/generate/externalFuncGenerator.h"
#include <map>
#include <set>
// #include <iostream>
frame::RegManager *reg_manager;
frame::Frags *frags;
bool compile_function = false;
std::map<std::string,std::set<uint64_t>> external_functions;


int main(int argc, char **argv) {
  std::string_view fname;
  std::unique_ptr<absyn::AbsynTree> absyn_tree;
  reg_manager = new frame::X64RegManager();
  frags = new frame::Frags();

  if (argc < 2) {
    fprintf(stderr, "usage: tiger-compiler file.tig\n");
    exit(1);
  }

  fname = std::string_view(std::string(argv[1])+".tig");

  {
    std::unique_ptr<err::ErrorMsg> errormsg;

    {
      // Preprocess
      TigerLog("-------====Preprocess=====-----\n");
      Preprocessor preprocessor(argv[1], std::string(std::string(argv[1])+".tig"));
      preprocessor.preprocess();
    }

    {
      // Lab 3: parsing
      TigerLog("-------====Parse=====-----\n");
      //if use fname, cannot open file ,strange bug
      Parser parser(std::string(std::string(argv[1])+".tig"), std::cerr);
      parser.parse();
      absyn_tree = parser.TransferAbsynTree();
      errormsg = parser.TransferErrormsg();
      // absyn_tree.get()->Print(stderr);
      // return 0;
    }

    // {
    //   // Lab 4: semantic analysis
    //   TigerLog("-------====Semantic analysis=====-----\n");
    //   sem::ProgSem prog_sem(std::move(absyn_tree), std::move(errormsg));
    //   prog_sem.SemAnalyze();
    //   absyn_tree = prog_sem.TransferAbsynTree();
    //   errormsg = prog_sem.TransferErrormsg();
    // }
    // if (errormsg->AnyErrors())
    //   return 1; // Don't continue if error occurrs

    {
      // Lab 5: escape analysis
      TigerLog("-------====Escape analysis=====-----\n");
      esc::EscFinder esc_finder(std::move(absyn_tree));
      esc_finder.FindEscape();
      absyn_tree = esc_finder.TransferAbsynTree();
    }

    {
      // Lab 5: translate IR tree
      TigerLog("-------====Translate=====-----\n");
      tr::ProgTr prog_tr(std::move(absyn_tree), std::move(errormsg));
      prog_tr.Translate();
      errormsg = prog_tr.TransferErrormsg();
      absyn_tree = prog_tr.TransferAbsynTree();
    }
    if (errormsg->AnyErrors())
      return 1; // Don't continue if error occurrs

    // std::cout<<"compile_function"<<std::endl;
    compile_function = true;
    {
      tr::ProgTr prog_tr(std::move(absyn_tree), std::move(errormsg));
      prog_tr.Translate();
      errormsg = prog_tr.TransferErrormsg();
    }


    if (errormsg->AnyErrors())
      return 1; // Don't continue if error occurrs
  }

  {
    // Output assembly
    output::AssemGen assem_gen(std::string(std::string(argv[1])+".tig"));
    assem_gen.GenAssem(true);
  }

  {
    //generate external functions in c
    ExternalFuncGenerator externalFuncGenerator(std::string(std::string(argv[1])+".cc"),external_functions);
    externalFuncGenerator.generate();
  }

  return 0;
}
