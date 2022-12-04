#ifndef TIGER_REGALLOC_REGALLOC_H_
#define TIGER_REGALLOC_REGALLOC_H_

#include "tiger/codegen/assem.h"
#include "tiger/codegen/codegen.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/regalloc/color.h"
#include "tiger/util/graph.h"
#include <map>

namespace ra {

class Result {
public:
  temp::Map *coloring_;
  assem::InstrList *il_;

  Result() : coloring_(nullptr), il_(nullptr) {}
  Result(temp::Map *coloring, assem::InstrList *il)
      : coloring_(coloring), il_(il) {}
  Result(const Result &result) = delete;
  Result(Result &&result) = delete;
  Result &operator=(const Result &result) = delete;
  Result &operator=(Result &&result) = delete;
  ~Result();
};

class RegAllocator {
  /* TODO: Put your lab6 code here */
public:
  RegAllocator(frame::Frame *frame,std::unique_ptr<cg::AssemInstr> assem_instr):
  frame_(frame),assem_instr_(std::move(assem_instr)),result_(std::make_unique<Result>()){}
  ~RegAllocator();
  void RegAlloc();
  std::unique_ptr<Result> TransferResult() { return std::move(result_); }

private:
  void LivenessAnalysis();
  void Build();
  void MakeWorklist();
  void Simplify();
  void DecrementDegree(const live::INodePtr &m);
  void AssignColors();

  live::INodePtr GetAlias(live::INodePtr n)const;

private:
  frame::Frame *frame_;
  std::unique_ptr<cg::AssemInstr> assem_instr_;
  std::unique_ptr<Result> result_;

  live::IGraphPtr live_graph_;
  fg::FGraphPtr flow_graph_;
  tab::Table<temp::Temp, live::INode> *temp2Inode;

  live::INodeList simplifyWorklist;
  // live::MoveList worklistMoves;

  std::map<live::INodePtr,int> degree;


  live::INodeList selectStack;
  live::INodeList coloredNodes;
  // live::INodeList precolored;
  std::map<live::INodePtr, temp::Temp*> color;
};

} // namespace ra

#endif