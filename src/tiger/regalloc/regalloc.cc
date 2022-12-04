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


    auto instr_list = assem_instr_->GetInstrList();
    for(const auto&node:flow_graph_->Nodes()->GetList()){
        if(fg::FlowGraphFactory::IsMove(node)){
            auto move_instr = static_cast<assem::MoveInstr*>(node->NodeInfo());
            auto src_node = temp2Inode->Look(move_instr->src_->GetList().front());
            auto dst_node = temp2Inode->Look(move_instr->dst_->GetList().front());
            if(color[src_node] == color[dst_node]){
                instr_list->Remove(move_instr);
            }
        }
    }

    result_->il_ = instr_list;
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
    flow_graph_ = flow_graph_factory.GetFlowGraph();
    auto live_graph_factory = live::LiveGraphFactory(flow_graph_);
    live_graph_factory.Liveness();
    auto live_graph = live_graph_factory.GetLiveGraph();
    live_graph_ = live_graph.interf_graph;


    //precolor
    temp2Inode = live_graph_factory.GetTempNodeMap();

    for(const auto &reg : reg_manager->Registers()->GetList()){
        auto node = temp2Inode->Look(reg);
        coloredNodes.Append(node);
        color[node] = reg;
    }
    //live_graph.moves;


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
                // fprintf(stderr,"%d cant have same color with %d\n",n->NodeInfo()->Int(),w->NodeInfo()->Int());
                okColors.remove(color[real_w]);
            }
        }
        coloredNodes.Append(n);
        // fprintf(stderr,"give %d color %d\n",n->NodeInfo()->Int(),okColors.front()->Int());
        color.emplace(n,okColors.front());   
    }
}

live::INodePtr RegAllocator::GetAlias(live::INodePtr n)const{
    return n;
}
} // namespace ra