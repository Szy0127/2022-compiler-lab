#include "tiger/liveness/flowgraph.h"
#include <list>
#include "flowgraph.h"
namespace fg {

void FlowGraphFactory::AssemFlowGraph() {
  /* TODO: Put your lab6 code here */

  // if instrj can be executed just after instri
  // instri->instrj will be added to the graph
  // std::map<temp::Label*, FNodePtr> label2node;
  std::list<std::pair<FNodePtr,assem::Targets*>> jumpsto;
  FNodePtr last_node = nullptr;
  FNodePtr current_node = nullptr;
  for(const auto &instr : instr_list_->GetList()) {
    current_node = flowgraph_->NewNode(instr);
    if (last_node){
      flowgraph_->AddEdge(last_node,current_node);
    }
    if(typeid(*instr) == typeid(assem::OperInstr)){
      auto jumps = static_cast<assem::OperInstr *>(instr)->jumps_;
      //current node should jumps to jumps
      if(jumps){
        //jump / cjump
        last_node = jumps->labels_->size()==1 ? nullptr : current_node;
        jumpsto.emplace_back(current_node,jumps);
        continue;
      }
    }
    if(typeid(*instr) == typeid(assem::LabelInstr)) {
      auto label = static_cast<assem::LabelInstr *>(instr)->label_;
      // label2node[label] = current_node;
      label_map_->Enter(label,current_node);
    } 
    last_node = current_node;
  }

  for (const auto &[node,j2] : jumpsto) {
    for (const auto &label : *(j2->labels_)) {
      auto target_node = label_map_->Look(label);
      assert(target_node);
      flowgraph_->AddEdge(node, target_node);
    }
  }
}
temp::TempList *FlowGraphFactory::GetUse(FNodePtr node){
  return node->NodeInfo()->Use();
}
temp::TempList *FlowGraphFactory::GetDef(FNodePtr node){
  return node->NodeInfo()->Def();
}
bool FlowGraphFactory::IsMove(FNodePtr node){
  auto instr = node->NodeInfo();
  return typeid(*instr)==typeid(assem::MoveInstr);
  // if(typeid(*instr)!=typeid(assem::MoveInstr)){
    // return false;
  // }
  // return static_cast<assem::MoveInstr*>(instr)->assem_=="movq `s0,`d0";
}

bool fg::FlowGraphFactory::IsCall(FNodePtr node) { 
  auto instr = node->NodeInfo();
  if(typeid(*instr)!=typeid(assem::OperInstr)){
    return false;
  }
  return !!static_cast<assem::OperInstr*>(instr)->pointer_map_; 
}
frame::StringFrag* fg::FlowGraphFactory::GetFrag(FNodePtr node) { 
  //should pass IsCall
  return static_cast<assem::OperInstr*>(node->NodeInfo())->pointer_map_; 
}

int FlowGraphFactory::GetArgInStack(FNodePtr node) { 
  return static_cast<assem::OperInstr*>(node->NodeInfo())->arg_in_stack; 
 }
} // namespace fg

namespace assem {

temp::TempList *LabelInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return new temp::TempList();
}

temp::TempList *MoveInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return dst_;
}

temp::TempList *OperInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return dst_;
}

temp::TempList *LabelInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return new temp::TempList();
}

temp::TempList *MoveInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return src_;
}

temp::TempList *OperInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return src_;
}
} // namespace assem
