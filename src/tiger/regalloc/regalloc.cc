#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"

extern frame::RegManager *reg_manager;

namespace ra {
/* TODO: Put your lab6 code here */
Result::~Result(){}

void RegAllocator::RegAlloc(){
    LivenessAnalysis();
    Build();
    MakeWorklist();
    while(true){
        if(!simplifyWorklist.GetList().empty()){
            Simplify();
        }else{
            break;
        }
    }
    AssignColors();    


    result_->il_ = assem_instr_->GetInstrList();

    auto reg_map = temp::Map::Empty();
    for(const auto &node : live_graph_->Nodes()->GetList()){
        reg_map->Enter(node->NodeInfo(), reg_manager->temp_map_->Look(color[node]));
    }
    result_->coloring_= reg_map;
}

RegAllocator::~RegAllocator(){}

void RegAllocator::LivenessAnalysis(){
    auto flow_graph_factory = fg::FlowGraphFactory(assem_instr_->GetInstrList());
    flow_graph_factory.AssemFlowGraph();
    auto live_graph_factory = live::LiveGraphFactory(flow_graph_factory.GetFlowGraph());
    live_graph_factory.Liveness();
    auto live_graph = live_graph_factory.GetLiveGraph();
    live_graph_ = live_graph.interf_graph;
    //live_graph.moves;

    //hard to understand
    // live_graph.interf_graph->Show(stderr,live_graph.interf_graph->Nodes(),nullptr);

}

void RegAllocator::Build(){
    auto live_nodes = live_graph_->Nodes()->GetList();
    for(const auto &node:live_nodes){
        degree.emplace(node,node->Degree());
    }

    // worklistMoves = live_graph->moves;
}

void RegAllocator::MakeWorklist(){
    auto initial = live_graph_->Nodes()->GetList();
    for(const auto &node:initial){
        simplifyWorklist.Append(node);
    }
}
void RegAllocator::Simplify(){
    auto node = simplifyWorklist.GetList().front();
    simplifyWorklist.DeleteNode(node);
    // list / stack
    selectStack.Prepend(node);
    for(const auto &adj:node->Adj()->GetList()) {
        DecrementDegree(adj);
    }
}
void RegAllocator::DecrementDegree(const live::INodePtr &m){
    degree[m]--;
}
void RegAllocator::AssignColors(){
    for(const auto &n:selectStack.GetList()){
        auto okColors = reg_manager->Registers()->GetList();
        okColors.remove(reg_manager->StackPointer());
        for(const auto &w:n->Adj()->GetList()){
            auto real_w = GetAlias(w);
            if(coloredNodes.Contain(real_w)){
                okColors.remove(color[real_w]);
            }
        }
        coloredNodes.Append(n);
        color[n] = okColors.front();   
    }
}

live::INodePtr RegAllocator::GetAlias(live::INodePtr n)const{
    return n;
}
} // namespace ra