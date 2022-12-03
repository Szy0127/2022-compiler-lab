#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"

extern frame::RegManager *reg_manager;

namespace ra {
/* TODO: Put your lab6 code here */
Result::~Result(){}

void RegAllocator::RegAlloc(){
    LivenessAnalysis();


    result_->il_ = assem_instr_->GetInstrList();
    result_->coloring_= temp::Map::Name();
}

RegAllocator::~RegAllocator(){}

void RegAllocator::LivenessAnalysis(){
    auto flow_graph_factory = fg::FlowGraphFactory(assem_instr_->GetInstrList());
    flow_graph_factory.AssemFlowGraph();
    auto live_graph_factory = live::LiveGraphFactory(flow_graph_factory.GetFlowGraph());
    live_graph_factory.Liveness();
    auto live_graph = live_graph_factory.GetLiveGraph();
    auto worklistMoves = live_graph.moves;

    //hard to understand
    live_graph.interf_graph->Show(stderr,live_graph.interf_graph->Nodes(),nullptr);

}
} // namespace ra