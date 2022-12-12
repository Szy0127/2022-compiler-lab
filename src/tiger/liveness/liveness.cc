#include "tiger/liveness/liveness.h"
#include<iostream>
extern frame::RegManager *reg_manager;

namespace live {

bool MoveList::Contain(INodePtr src, INodePtr dst) {
  return std::any_of(move_list_.cbegin(), move_list_.cend(),
                     [src, dst](std::pair<INodePtr, INodePtr> move) {
                       return move.first == src && move.second == dst;
                     });
}

void MoveList::Delete(INodePtr src, INodePtr dst) {
  assert(src && dst);
  auto move_it = move_list_.begin();
  for (; move_it != move_list_.end(); move_it++) {
    if (move_it->first == src && move_it->second == dst) {
      break;
    }
  }
  move_list_.erase(move_it);
}

MoveList *MoveList::Union(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : move_list_) {
    res->move_list_.push_back(move);
  }
  for (auto move : list->GetList()) {
    if (!res->Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

MoveList *MoveList::Intersect(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : list->GetList()) {
    if (Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

void LiveGraphFactory::LiveMap() {
  /* TODO: Put your lab6 code here */

  //compute in/out liveness according to use/def
  auto graph_node_list = flowgraph_->Nodes()->GetList();
  for (const auto &node:graph_node_list) {
    in_->Enter(node, new temp::TempList());
    out_->Enter(node, new temp::TempList());
  }
  bool not_change = false;
  while(!not_change){
    not_change = true;
    for(const auto&node:graph_node_list){
      auto old_in = in_->Look(node);
      auto old_out = out_->Look(node);
      auto new_in = TempList_Union(fg::FlowGraphFactory::GetUse(node),
                                  TempList_Diff(old_out,fg::FlowGraphFactory::GetDef(node)));
      auto succ_list = node->Succ()->GetList();
      temp::TempList* new_out = nullptr;
      for(const auto&succ:succ_list){
        auto succ_in = in_->Look(succ);
        new_out = TempList_Union(new_out,succ_in);
      }
      in_->Enter(node,new_in);
      if(new_out){
        out_->Enter(node,new_out);
      }
      if(!TempList_Same(old_in,new_in) || !TempList_Same(old_out,new_out)){
        not_change = false;
      }
    }
  }

  //show outs
  // temp::Map *color = temp::Map::LayerMap(reg_manager->temp_map_, temp::Map::Name());
  // for(const auto&temp:reg_manager->Registers()->GetList()){
  //   fprintf(stderr,"%d" ,temp->Int());
  // }
  // fprintf(stderr,"\n");
  // for (const auto &node:graph_node_list) {
  //   auto live_out_list = out_->Look(node)->GetList();
  //   auto instr = static_cast<assem::Instr*>(node->NodeInfo());
  //   instr->Print(stderr,color);
  //   fprintf(stderr,"defs:");
  //   for(const auto&temp:fg::FlowGraphFactory::GetDef(node)->GetList()){
  //     fprintf(stderr,"%d ",temp->Int());
  //   }
  //   fprintf(stderr,"\nlive-out:");
  //   for(const auto&temp:live_out_list){
  //     fprintf(stderr,"%d ",temp->Int());
  //   }
  //   fprintf(stderr,"\n");
  // }

}

void LiveGraphFactory::InterfGraph() {
  /* TODO: Put your lab6 code here */
  //only need out_
  //all temps in out,add each pair to graph
  
  auto rsp = reg_manager->StackPointer();
  auto graph_node_list = flowgraph_->Nodes()->GetList();
  for(const auto &node:graph_node_list){
    auto out_temps = out_->Look(node)->GetList();
    for(const auto &temp:out_temps){
      if (!temp_node_map_->Look(temp)){
        auto new_live_node = live_graph_.interf_graph->NewNode(temp);
        temp_node_map_->Enter(temp,new_live_node);
      }
      for(const auto&temp:fg::FlowGraphFactory::GetDef(node)->GetList()){
        if (!temp_node_map_->Look(temp)){
          auto new_live_node = live_graph_.interf_graph->NewNode(temp);
          temp_node_map_->Enter(temp,new_live_node);
        }
      }
    }
  }
  for(const auto &node:graph_node_list) {
    auto live_out = out_->Look(node);
    auto defs = fg::FlowGraphFactory::GetDef(node)->GetList();
    if(fg::FlowGraphFactory::IsMove(node)){
      auto def = defs.front();
      if(def==rsp){
        continue;
      }
      auto node_def = temp_node_map_->Look(def);
      auto uses = fg::FlowGraphFactory::GetUse(node);
      for(const auto &out:TempList_Diff(live_out,uses)->GetList()){
        auto node_out = temp_node_map_->Look(out);
        //add edge not symetric but adj will return both succ and pred
        if(node_def!=node_out){
          live_graph_.interf_graph->AddEdge(node_def, node_out);
        }
      }
      // for register allocation
      // mov t_src,t_dst
      // mov node_use,node_def
      auto use = uses->GetList().front();
      if(use==rsp){
        continue;
      }
      auto node_use = temp_node_map_->Look(use);
      //src dst
      //|| !live_graph_.worklistMoves->Contain(node_def,node_use)
      if(!live_graph_.worklistMoves->Contain(node_use,node_def) || !live_graph_.worklistMoves->Contain(node_def,node_use) ){
        live_graph_.worklistMoves->Append(node_use,node_def);
        // live_graph_.worklistMoves->Append(node_def,node_use);
        if(live_graph_.moveList->count(node_use)){
          live_graph_.moveList->at(node_use)->Append(node_use,node_def);
        }else{
          auto moves = new MoveList();
          moves->Append(node_use,node_def);
          live_graph_.moveList->emplace(node_use,moves);
        }
        if(live_graph_.moveList->count(node_def)){
          live_graph_.moveList->at(node_def)->Append(node_use,node_def);
        }else{
          auto moves = new MoveList();
          moves->Append(node_use,node_def);
          live_graph_.moveList->emplace(node_def,moves);
        }
      }
    }else{
      for(const auto &def:defs){
        if(def==rsp){
          continue;
        }
        auto node_def = temp_node_map_->Look(def);
        for(const auto &out:live_out->GetList()){
          if(out==rsp){
            continue;
          }
          auto node_out = temp_node_map_->Look(out);
          if(node_def!=node_out){
            live_graph_.interf_graph->AddEdge(node_def,node_out);
          }
        }
      }
    }
  }
  
  // live_graph_.interf_graph->Show(stderr,live_graph_.interf_graph->Nodes(),[](temp::Temp*t){fprintf(stderr,"temp:%d",t->Int());});
}

void LiveGraphFactory::Liveness() {
  LiveMap();
  InterfGraph();
}

bool LiveGraphFactory::TempList_Contain(temp::TempList*list,temp::Temp*target){
  if(!list || !target){
    return false;
  }
  auto l = list->GetList();
  return std::find(l.begin(),l.end(),target) != l.end();
}
temp::TempList* LiveGraphFactory::TempList_Union(temp::TempList *l,temp::TempList *r){
  if(!l && !r) {
    return new temp::TempList();
  }
  auto union_list = new temp::TempList();
  if(l){
    for(const auto &t:l->GetList()){
      union_list->Append(t);
    }
  }
  for(const auto &t : r->GetList()) {
    if(!TempList_Contain(union_list, t)){
      union_list->Append(t);
    }
  }
  return union_list;
}


temp::TempList* LiveGraphFactory::TempList_Diff(temp::TempList *l,temp::TempList *r){
  if(!l) {
    return new temp::TempList();
  }
  auto diff_list = new temp::TempList();
  if(!r){
    for(const auto &t : l->GetList()) {
        diff_list->Append(t);
    }
    return diff_list;
  }
  for(const auto &t : l->GetList()) {
    //poor performance 
    if(!TempList_Contain(r, t)){
      diff_list->Append(t);
    }
  }
  return diff_list;
}
bool LiveGraphFactory::TempList_Same(temp::TempList *l,temp::TempList *r){
  auto l_empty = !l || l->GetList().empty();
  auto r_empty = !r || r->GetList().empty();
  if(l_empty && r_empty){
    return true;
  }
  if(l_empty || r_empty){
    return false;
  }
  for(const auto&t:l->GetList()){
    if(!TempList_Contain(r,t)){
      return false;
    }
  }
  for(const auto&t:r->GetList()){
    if(!TempList_Contain(l,t)){
      return false;
    }
  }
  return true;
}
} // namespace live
