struct Node {
    float T;
    char type; // c = conduction, g = ghost node (used for insulated points), t = constant temperature, s = surrounding temp (convection)
};

struct Coord {
    int x;
    int y;
};

float nodeHT (int x0, int y0, int x, int y, float dx, float E, float K, float H, int n, __global struct Node* nodes){
	// Determines and returns the change in temperature between 2 nodes
	
	if (nodes[x+y*(n+1)].type == 'c' || nodes[x+y*(n+1)].type == 't')
		return E*K*(nodes[x+y*(n+1)].T-nodes[x0+y0*(n+1)].T);
	else if(nodes[x+y*(n+1)].type == 's')
		return -E*dx*H*(nodes[x0+y0*(n+1)].T-nodes[x+y*(n+1)].T);
	else // Insulation (ghost node - no heat transfer)
		return 0;
}

__kernel void thermoNode(float dt, float dx, float K, float H, float C, int n,int totalCnodes,__global struct Node* nodes,__global struct Node* newNodes,__global struct Coord* list)
{
    int g_id = get_global_id(0); // global id of work item
	//int l_id = get_local_id(0); // local id of work item
	int n_threads = get_global_size(0); // global size of index space
    float E = dt/(C*dx*dx);


    // numC represents the number of coordinates/tasks to iterate through.
    // indexC represents a work item's starting index within list.
    int i,j,x,y, min, r, numC, indexC;
    float t;
    
    min = totalCnodes / n_threads;
    r = totalCnodes % n_threads;
    
    numC = min;
    
	if (g_id < r){
		numC++;
		indexC = (min+1)*g_id;
	}else{
		indexC = (min+1)*r + (g_id-r)*min;
	}

	// Classes are not supported. Pointers inside structs are not supported.
    // Update all temperatures for tasked nodes.
    for (i = 0; i < numC; i++){
    	x = list[indexC + i].x;
		y = list[indexC + i].y;
		t = nodes[x+y*(n+1)].T;
		
		t += nodeHT (x, y, x - 1, y, dx, E, K, H, n, nodes);
		t += nodeHT (x, y, x + 1, y, dx, E, K, H, n, nodes);
		t += nodeHT (x, y, x, y - 1, dx, E, K, H, n, nodes);
		t += nodeHT (x, y, x, y + 1, dx, E, K, H, n, nodes);
		newNodes[x+y*(n+1)].T = t;
    }
    
    barrier(CLK_GLOBAL_MEM_FENCE);
    
    for (i = 0; i < numC; i++){
    	x = list[indexC + i].x;
		y = list[indexC + i].y;
		nodes[x+y*(n+1)].T = newNodes[x+y*(n+1)].T;
    }
}