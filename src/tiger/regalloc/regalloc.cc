#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"
#include "regalloc.h"
#include<iostream>
#include <sstream>

extern frame::RegManager *reg_manager;

namespace ra {
/* TODO: Put your lab6 code here */
Result::~Result(){}

void RegAllocator::RegAlloc(){
    // std::cout<<"---frame reg begin---"<<std::endl;
    bool success = false;
    while(!success){
        // std::cout<<"---reg begin---"<<std::endl;
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
        //exam
        // for(const auto&n:live_graph_->Nodes()->GetList()){
        //     if(spillNodes.Contain(n)){
        //         continue;
        //     }
        //     for(const auto&m:n->Adj()->GetList()){
        //         if(color[n] == color[m]){
        //             std::cout<<"error,cant draw same color of"<<n->NodeInfo()->Int()<<"and"<<m->NodeInfo()->Int()<<std::endl;
        //         }
        //     }
        // }
        if(!spillNodes.Empty()){
            RewriteProgram();
        }else{
            success = true;
        }
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
    selectStack = new live::INodeList();
    coalescedNodes = new live::INodeList();
    activeMoves = new live::MoveList();

    simplifyWorklist.Clear();
    spillWorklist.Clear();
    freezeWorklist.Clear();
    frozenMoves.Clear();
    coalescedMoves.Clear();
    constrainedMoves.Clear();
    degree.clear();
    coloredNodes.Clear();
    spillNodes.Clear();
    color.clear();
    alias.clear();


    auto live_nodes = live_graph_->Nodes()->GetList();
    for(const auto &node:live_nodes){
        degree.emplace(node,node->Degree());
    }

    for(const auto &reg : reg_manager->Registers()->GetList()){
        auto node = temp2Inode->Look(reg);
        coloredNodes.Append(node);
        color[node] = reg;
        if(degree.count(node)){
            degree[node] = INT64_MAX;
        }else{
            degree.emplace(node,INT64_MAX);
        }
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
    // std::cout<<"stack push"<<n->NodeInfo()->Int()<<std::endl;
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
    if(!u->Adj(v)&& u != v) {
        // std::cout<<"add edge"<<u->NodeInfo()->Int()<<" "<<v->NodeInfo()->Int()<<std::endl;
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
    // std::cout<<k<<std::endl;
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
    // std::cout<<"---assign---"<<std::endl;
    for(const auto &n:selectStack->GetList()){
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
            // std::cout<<"spill:"<<n->NodeInfo()->Int()<<"adj:"<<n->Adj()->GetList().size()<<std::endl;
        }else{
            coloredNodes.Append(n);
            color.emplace(n,okColors.front());   
            // std::cout<<"draw:"<<n->NodeInfo()->Int()<<",color:"<<okColors.front()->Int()<<std::endl;
        }
    }
    for(const auto&n:coalescedNodes->GetList()){
        // std::cout<<"draw:"<<n->NodeInfo()->Int()<<" "<<GetAlias(n)->NodeInfo()->Int()<<std::endl;
        if(!color.count(GetAlias(n))){
            continue;
        }
        // std::cout<<color[GetAlias(n)]->Int()<<std::endl;
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
    auto spill_list = spillWorklist.GetList();
    for(const auto&m:spill_list){
        if(!spill_introduced_temps.Contain(m->NodeInfo())){
            spillWorklist.DeleteNode(m);
            simplifyWorklist.Append(m);
            FreezeMoves(m);
            return;
        }
    }
    auto m = spill_list.front();
    spillWorklist.DeleteNode(m);
    simplifyWorklist.Append(m);
    FreezeMoves(m);
}
void RegAllocator::RewriteProgram(){
    // std::cout<<"before rewrite:"<<assem_instr_->GetInstrList()->GetList().size()<<std::endl;
    auto rsp = reg_manager->StackPointer();
    for(const auto&spill_node:spillNodes.GetList()){
        auto spill_temp = spill_node->NodeInfo();
        // std::cout<<"spilled:"<<spill_temp->Int()<<std::endl;
        auto frame_access = static_cast<frame::InFrameAccess*>(frame_->AllocLocal(true));
        auto instr_list_ = assem_instr_->GetInstrList();
        auto end = instr_list_->GetList().end();
        auto instr_it = instr_list_->GetList().begin();
        for(;instr_it!=end;instr_it++){
            temp::TempList* src_list = nullptr;
            temp::TempList* dst_list = nullptr;

            if(typeid(**instr_it) == typeid(assem::MoveInstr)){
                auto ins = static_cast<assem::MoveInstr*>(*instr_it);
                src_list = ins->src_;
                dst_list = ins->dst_;
            }
            if(typeid(**instr_it) == typeid(assem::OperInstr)){
                auto ins = static_cast<assem::OperInstr*>(*instr_it);
                src_list = ins->src_;
                dst_list = ins->dst_;
            }
            
            //load
            if(src_list && src_list->Contain(spill_temp)){
                auto t = temp::TempFactory::NewTemp();
                // std::cout<<"write src,new temp:"<<t->Int()<<std::endl;
                spill_introduced_temps.Append(t);
                src_list->Replace(spill_temp,t);
                std::stringstream assem;
                assem << "movq ("<< frame_->GetLabel() << "_framesize" << frame_access->offset << ")(`s0),`d0";
                instr_list_->Insert(
                    instr_it, new assem::OperInstr(
                        assem.str(),
                        new temp::TempList(t),
                        new temp::TempList(rsp),
                        nullptr
                    )
                );
            }
            //store
            if(dst_list && dst_list->Contain(spill_temp)){
                auto t = temp::TempFactory::NewTemp();
                // std::cout<<"write dst,new temp:"<<t->Int()<<std::endl;
                spill_introduced_temps.Append(t);
                dst_list->Replace(spill_temp,t);
                std::stringstream assem;
                assem << "movq `s0,("<< frame_->GetLabel() << "_framesize" << frame_access->offset << ")(`d0)";
                instr_list_->Insert(
                    std::next(instr_it), new assem::OperInstr(
                        assem.str(),
                        new temp::TempList(rsp),
                        new temp::TempList(t),
                        nullptr
                    )
                );
            }
        }
    }
    // std::cout<<"after rewrite:"<<assem_instr_->GetInstrList()->GetList().size()<<std::endl;
}
} // namespace ra