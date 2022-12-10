#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"
#include "regalloc.h"
#include<iostream>

extern frame::RegManager *reg_manager;

namespace ra {
/* TODO: Put your lab6 code here */
Result::~Result(){}

void RegAllocator::RegAlloc(){
    LivenessAnalysis();
    Build();
    MakeWorklist();
    while(true){
        if(!simplifyWorklist.Empty()){
            Simplify();
            continue;
        }
        if(!worklistMoves->Empty()){
            Coalesce();
            continue;
        }
        if(!freezeWorklist.Empty()){
            Freeze();
            continue;
        }
        if(!spillWorklist.Empty()){
            SelectSpill();
            continue;
        }
        break;
    }
    AssignColors();  
    if(!spillNodes.Empty()){
        std::cout<<"error?"<<std::endl;
    }  


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
    worklistMoves = live_graph.worklistMoves;
    moveList = live_graph.moveList;

    //precolor
    temp2Inode = live_graph_factory.GetTempNodeMap();


}

void RegAllocator::Build(){ 


    for(const auto &reg : reg_manager->Registers()->GetList()){
        auto node = temp2Inode->Look(reg);
        coloredNodes.Append(node);
        color[node] = reg;
    }

    auto live_nodes = live_graph_->Nodes()->GetList();
    for(const auto &node:live_nodes){
        degree.emplace(node,node->Degree());
    }


}

void RegAllocator::MakeWorklist(){
    auto initial = live_graph_->Nodes()->GetList();
    for(const auto &node:initial){
        if(coloredNodes.Contain(node)){//precolored
            continue;
        }
        if(degree[node] >= K){
            spillWorklist.Append(node);
            continue;
        }
        if(MoveRelated(node)){
            freezeWorklist.Append(node);
            continue;
        }
        simplifyWorklist.Append(node);
    }
}
void RegAllocator::Simplify(){
    // std::cout<<"Simplify"<<std::endl;
    auto n = simplifyWorklist.GetList().front();
    simplifyWorklist.DeleteNode(n);
    // list / stack
    selectStack->Prepend(n);
    for(const auto &adj:Adjacent(n)->GetList()) {
        DecrementDegree(adj);
    }
}
void RegAllocator::Coalesce() {
    auto [x,y] = worklistMoves->Pop();
    auto xx = GetAlias(x);
    auto yy = GetAlias(y);
    live::INodePtr u, v;
    if(coloredNodes.Contain(yy)){
        u = yy; // u,v=y,x wrong
        v = xx;
    }else{
        u = xx;
        v = yy;
    }
    if(u==v){
        coalescedMoves.Append(x,y);
        AddWorkList(u);
        return;
    }

    if(coloredNodes.Contain(v) || u->Adj(v)){
        constrainedMoves.Append(x,y);
        AddWorkList(u);
        AddWorkList(v);
        return;
    }

    bool george_flag = true;
    for(const auto&t:Adjacent(v)->GetList()){
        if(!OK(t,u)){
            george_flag = false;
            break;
        }
    }
    if((coloredNodes.Contain(u) && george_flag)|| (!coloredNodes.Contain(u) && Conservative(Adjacent(u)->Union(Adjacent(v))))){
        coalescedMoves.Append(x,y);
        // std::cout<<"combine"<<u->NodeInfo()->Int()<<" "<<v->NodeInfo()->Int()<<std::endl;
        Combine(u,v);
        AddWorkList(u);
        return;
    }
    activeMoves->Append(x,y);
}
void RegAllocator::Freeze(){
    auto u = freezeWorklist.GetList().front();
    freezeWorklist.DeleteNode(u);
    simplifyWorklist.Append(u);
    FreezeMoves(u);
}
void RegAllocator::FreezeMoves(live::INodePtr u){
    live::INodePtr v;
    for(auto[x,y]:NodeMoves(u)->GetList()){
        if(GetAlias(y) == GetAlias(u)){
            v = GetAlias(x);
        }else{
            v = GetAlias(y);
        }
        activeMoves->Delete(x,y);
        frozenMoves.Append(x,y);
        if(NodeMoves(v)->Empty()&&degree[v]<K){
            freezeWorklist.DeleteNode(v);
            simplifyWorklist.Append(v);
        }
    }
}
void RegAllocator::AddWorkList(live::INodePtr n){
    // std::cout<<"addworklist"<<std::endl;
    if(!coloredNodes.Contain(n) && !MoveRelated(n)&& degree[n] < K){
        freezeWorklist.DeleteNode(n);
        simplifyWorklist.Append(n);
    }
}
void RegAllocator::Combine(live::INodePtr u,live::INodePtr v){
    if(freezeWorklist.Contain(v)){
        freezeWorklist.DeleteNode(v);
    }else{
        spillWorklist.DeleteNode(v);
    }
    coalescedNodes->Append(v);
    if(alias.count(v)){
        alias[v] = u;
    }else{
        alias.emplace(v,u);
    }
    (*moveList)[u] = (*moveList)[u]->Union((*moveList)[v]);
    EnableMoves(v);
    for(const auto &t:Adjacent(v)->GetList()){
        AddEdge(t,u);
        DecrementDegree(t);
    }
    if(degree[u]>=K && freezeWorklist.Contain(u)){
        freezeWorklist.DeleteNode(u);
        spillWorklist.Append(u);
    }

}
void RegAllocator::EnableMoves(const live::INodePtr &node){
    // std::cout<<"enable movees"<<std::endl;
    for(const auto&[src,dst]:NodeMoves(node)->GetList()){
        if(activeMoves->Contain(src,dst)){
            activeMoves->Delete(src,dst);
            if(!worklistMoves->Contain(src,dst)){
                worklistMoves->Append(src,dst);
            }
        }
    }
}

void RegAllocator::EnableMoves(live::INodeList *nodes){
    for(const auto&n:nodes->GetList()){
        EnableMoves(n);
    }
}

void RegAllocator::AddEdge(live::INodePtr u,live::INodePtr v){
    // std::cout<<"add edge"<<u->NodeInfo()->Int()<<" "<<v->NodeInfo()->Int()<<std::endl;
    if(!u->Adj(v)&& u != v) {
        live_graph_->AddEdge(u, v);
        live_graph_->AddEdge(v, u);
        if(!coloredNodes.Contain(u)){
            degree[u]++;
        }
        if(!coloredNodes.Contain(v)){
            degree[v]++;
        }
    }
}
bool RegAllocator::OK(const live::INodePtr &t, const live::INodePtr &r) {
    return degree[t]<K || coloredNodes.Contain(t) ||  t->Adj(r);
}
bool RegAllocator::Conservative(live::INodeList *nodes) {
    auto k = 0;
    for(const auto&n:nodes->GetList()){
        if(degree[n]>=K){
            k++;
        }
    }
    return k<K;
}
void RegAllocator::DecrementDegree(const live::INodePtr &m) { 
    degree[m]--; 
    if(degree[m] == K-1){
        auto list = Adjacent(m);
        list->Append(m);
        EnableMoves(list);
        spillWorklist.DeleteNode(m);
        if(MoveRelated(m)){
            freezeWorklist.Append(m);
        }else{
            simplifyWorklist.Append(m);
        }
    }    
}
void RegAllocator::AssignColors(){
    for(const auto &n:selectStack->GetList()){
        if(color.count(n)){
            continue;
        }
        auto okColors = reg_manager->Registers()->GetList();
        okColors.remove(reg_manager->StackPointer());
        for(const auto &w:n->Adj()->GetList()){
            auto real_w = GetAlias(w);
            if(coloredNodes.Contain(real_w)){
                okColors.remove(color[real_w]);
            }
        }
        if(okColors.empty()){
            spillNodes.Append(n);
            // std::cout<<"spill:"<<n->NodeInfo()->Int()<<std::endl;
        }else{
            coloredNodes.Append(n);
            color.emplace(n,okColors.front());   
            // std::cout<<n->NodeInfo()->Int()<<" "<<okColors.front()->Int()<<std::endl;
        }
    }
    for(const auto&n:coalescedNodes->GetList()){
        // std::cout<<n->NodeInfo()->Int()<<" "<<GetAlias(n)->NodeInfo()->Int()<<" "<<color[GetAlias(n)]->Int()<<std::endl;
        if(color.count(n)){
            color[n] = color[GetAlias(n)];
        }else{
            color.emplace(n,color[GetAlias(n)]);
        }
    }
}

live::INodePtr RegAllocator::GetAlias(live::INodePtr n){
    if(coalescedNodes->Contain(n)){
        return GetAlias(alias[n]);
    }
    return n;
}

live::MoveList *RegAllocator::NodeMoves(const live::INodePtr &n) const {
    // return new live::MoveList();
    if(!moveList->count(n)){
        return new live::MoveList();
    }
    return moveList->at(n)->Intersect(activeMoves->Union(worklistMoves));
}

bool RegAllocator::MoveRelated(const live::INodePtr &n) const {
    return !NodeMoves(n)->Empty();
}
live::INodeList* RegAllocator::Adjacent(const live::INodePtr &n){
    return n->Adj()->Diff(selectStack->Union(coalescedNodes));
}

void RegAllocator::SelectSpill(){
    auto m = spillWorklist.GetList().front();
    spillWorklist.DeleteNode(m);
    simplifyWorklist.Append(m);
    FreezeMoves(m);
}
} // namespace ra