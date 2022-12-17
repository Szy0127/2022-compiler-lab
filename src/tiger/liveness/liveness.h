#ifndef TIGER_LIVENESS_LIVENESS_H_
#define TIGER_LIVENESS_LIVENESS_H_

#include "tiger/codegen/assem.h"
#include "tiger/frame/x64frame.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/flowgraph.h"
#include "tiger/util/graph.h"
#include<map>
namespace live {

using INode = graph::Node<temp::Temp>;
using INodePtr = graph::Node<temp::Temp>*;
using INodeList = graph::NodeList<temp::Temp>;
using INodeListPtr = graph::NodeList<temp::Temp>*;
using IGraph = graph::Graph<temp::Temp>;
using IGraphPtr = graph::Graph<temp::Temp>*;

class MoveList {
public:
  MoveList() = default;

  [[nodiscard]] const std::list<std::pair<INodePtr, INodePtr>> &
  GetList() const {
    return move_list_;
  }
  void Append(INodePtr src, INodePtr dst) { move_list_.emplace_back(src, dst); }
  bool Empty(){return move_list_.empty();}
  bool Contain(INodePtr src, INodePtr dst);
  void Delete(INodePtr src, INodePtr dst);
  void Prepend(INodePtr src, INodePtr dst) {
    move_list_.emplace_front(src, dst);
  }
  void Clear(){move_list_.clear();}
  std::pair<INodePtr, INodePtr> Pop(){auto ret = move_list_.front();move_list_.pop_front();return ret;}
  MoveList *Union(MoveList *list);
  MoveList *Intersect(MoveList *list);

private:
  std::list<std::pair<INodePtr, INodePtr>> move_list_;
};

struct LiveGraph {
  IGraphPtr interf_graph;
  MoveList *worklistMoves;
  // tab::Table<INode,MoveList> *moveList;//Enter(Look().Append)  poor performance
  std::map<INodePtr,MoveList*> *moveList;

  LiveGraph(IGraphPtr interf_graph, MoveList *worklistMoves,std::map<INodePtr,MoveList*> *moveList)
      : interf_graph(interf_graph), worklistMoves(worklistMoves),moveList(moveList) {}
};

class LiveGraphFactory {
public:
  explicit LiveGraphFactory(fg::FGraphPtr flowgraph)
      : flowgraph_(flowgraph), live_graph_(new IGraph(), new MoveList(),new std::map<INodePtr,MoveList*>()),
        in_(std::make_shared<graph::Table<assem::Instr, temp::TempList>>()),
        out_(std::make_unique<graph::Table<assem::Instr, temp::TempList>>()),
        temp_node_map_(new tab::Table<temp::Temp, INode>()) {}
  void Liveness();
  LiveGraph GetLiveGraph() { return live_graph_; }
  tab::Table<temp::Temp, INode> *GetTempNodeMap() { return temp_node_map_; }
  [[nodiscard]] std::shared_ptr<graph::Table<assem::Instr, temp::TempList>> GetLiveIn()const{return in_;}

private:
  fg::FGraphPtr flowgraph_;
  LiveGraph live_graph_;

  std::shared_ptr<graph::Table<assem::Instr, temp::TempList>> in_;
  std::unique_ptr<graph::Table<assem::Instr, temp::TempList>> out_;
  tab::Table<temp::Temp, INode> *temp_node_map_;

  void LiveMap();
  void InterfGraph();

  [[nodiscard]] static bool TempList_Contain(temp::TempList *list,temp::Temp* target);
  [[nodiscard]] static temp::TempList* TempList_Union(temp::TempList *l,temp::TempList *r);//l+r
  [[nodiscard]] static temp::TempList* TempList_Diff(temp::TempList *l,temp::TempList *r);//l-r
  [[nodiscard]] static bool TempList_Same(temp::TempList *l,temp::TempList *r);//l+r
};

} // namespace live

#endif