#include "graph.h"

bool resolve_node(DepNode *node, DepNodeRegistry *registry);
void run_dependency_resolver(DepGraph *graph, DepNodeRegistry *registry);