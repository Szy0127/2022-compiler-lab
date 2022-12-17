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
  void Coalesce();
  void Freeze();
  void SelectSpill();
  void DecrementDegree(const live::INodePtr &m);
  void AssignColors();

  void RewriteProgram();

  live::INodePtr GetAlias(live::INodePtr n);

  bool MoveRelated(const live::INodePtr &n)const;
  live::MoveList* NodeMoves(const live::INodePtr &n)const;
  void AddWorkList(live::INodePtr n);
  bool OK(const live::INodePtr &t,const live::INodePtr &r);//george
  bool Conservative(live::INodeList* nodes);//briggs

  live::INodeList* Adjacent(const live::INodePtr &n);
  void Combine(live::INodePtr u,live::INodePtr v);
  void EnableMoves(const live::INodePtr &node);
  void EnableMoves(live::INodeList *nodes);
  void AddEdge(live::INodePtr u,live::INodePtr v);
  void FreezeMoves(live::INodePtr u);
private:
  frame::Frame *frame_;
  std::unique_ptr<cg::AssemInstr> assem_instr_;
  std::unique_ptr<Result> result_;

  live::IGraphPtr live_graph_;
  std::shared_ptr<graph::Table<assem::Instr, temp::TempList>> in_;//for pointer_map before call

  fg::FGraphPtr flow_graph_;
  tab::Table<temp::Temp, live::INode> *temp2Inode;

  live::INodeList simplifyWorklist;
  live::INodeList spillWorklist;
  live::INodeList freezeWorklist;
  live::MoveList *worklistMoves;
  live::MoveList *activeMoves;
  // live::MoveList frozenMoves;
  // live::MoveList coalescedMoves;
  // live::MoveList constrainedMoves;
  std::map<live::INodePtr,live::MoveList*> *moveList;

  std::map<live::INodePtr,int64_t> degree;


  live::INodeList *selectStack;
  live::INodeList *coalescedNodes;
  live::INodeList coloredNodes;
  live::INodeList spillNodes;
  // live::INodeList precolored;
  std::map<live::INodePtr, temp::Temp*> color;
  std::map<live::INodePtr, live::INodePtr> alias;

  temp::TempList spill_introduced_temps;


  static const unsigned int K=15;
};

} // namespace ra

#endif