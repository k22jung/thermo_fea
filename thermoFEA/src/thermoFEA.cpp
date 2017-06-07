//============================================================================
// Name        : GPUtest.cpp
// Author      :
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#include <iostream>
#include <CL\cl.hpp>
#include <CL\opencl.h>
#include <fstream>
#include <streambuf>
#include <string>
#include <sstream>
#include <ctime>
//#include "LinkedList.hpp"
//http://stackoverflow.com/questions/21318112/how-to-prepare-eclipse-for-opencl-programming-intel-opencl-sdk-installed-in-li

//http://www.browndeertechnology.com/docs/BDT_OpenCL_Tutorial_NBody-rev3.html#algorithm
//http://stackoverflow.com/questions/15554591/initialise-an-opencl-object
//http://stackoverflow.com/questions/10916093/an-error-of-opencl-kernel-compile
/* nbody.c version #1 */

struct Coord {
    cl_int x;
    cl_int y;
};

inline void checkErr(cl_int err, const char * name) {
	if (err != CL_SUCCESS) {
	std::cerr << "ERROR: " << name  << " (" << err << ")" << std::endl;
	exit(EXIT_FAILURE);
	}
}

#pragma pack(16)
//
struct Node {
    cl_float T;
    cl_char type; // c = conduction, g = ghost node (used for insulated points), t = constant temperature, s = surrounding temp (convection)
};

//struct NewData{
//	cl_float T;
//	cl_int x;
//	cl_int y;
//};



#pragma pack(16)


const int workers = 20;
const int groupSize = 1;
const cl_float K = 205;
const cl_float dx = 0.001;
const cl_float dt = 0.001;
const cl_float dt_display = 0.01;
const int count = dt_display/dt;
const cl_float H = 50000;
const cl_float totalTime = 10;
const int iterations = totalTime/dt;
const cl_float Density = 2720;
const cl_float Cspec = 921.096;
//http://www.engineeringtoolbox.com/metal-alloys-densities-d_50.html
//http://www.engineersedge.com/materials/specific_heat_capacity_of_metals_13259.htm
//https://en.wikipedia.org/wiki/Volumetric_heat_capacity
const cl_float C = Density*Cspec;

void buildKernel (){
	cl_uint cl_platformsN = 0;
	cl_platform_id *cl_platformIDs = NULL;

	clGetPlatformIDs (0, NULL, &cl_platformsN);

	cl_platformIDs = (cl_platform_id*)malloc( cl_platformsN * sizeof(cl_platform_id));
	clGetPlatformIDs(cl_platformsN, cl_platformIDs, NULL);

	cl_int status = CL_SUCCESS;
	cl_device_id device;    // Compute device
	cl_context context;     // Compute context

	clGetDeviceIDs(cl_platformIDs[0], CL_DEVICE_TYPE_GPU, 1, &device, NULL);
	context = clCreateContext(NULL, 1, &device, NULL, NULL, &status);

	 std::ifstream GPUtestfile("thermoNode.cl");
	checkErr(GPUtestfile.is_open() ? CL_SUCCESS:-1, "thermoNode.cl");
	std::string src((std::istreambuf_iterator<char>(GPUtestfile)), (std::istreambuf_iterator<char>()));
	cl::Program::Sources sources(1, std::make_pair(src.c_str(), src.length()+1));


	cl_program program = clCreateProgramWithSource(context, 1,
												   (const char**)&src, NULL,
												   &status);
	status = clBuildProgram(program, 0, NULL, "-I .\\src\\", NULL, NULL);

	size_t paramValueSize = 1024 * 1024, param_value_size_ret;
	char *paramValue;
	paramValue = (char*)calloc(paramValueSize, sizeof(char));
		status = clGetProgramBuildInfo( program,
									device,
									CL_PROGRAM_BUILD_LOG,
									paramValueSize,
									paramValue,
									&param_value_size_ret);
	printf("%s\n", paramValue);
	return;
}

bool stability_check (double stability, const cl_float dt){
	if (stability <= dt){
		std::cout << "Parameters do not guarantee stability (" << stability << " <= " << dt << ")" << std::endl;
		return false;
	}
	return true;
}

void readNodes(std::string filename, Node* &nodes, Coord* &list, int* m, int* n, int* totalCnodes) {
	std::string line;
	std::ifstream nodeFile(filename);


	if(!nodeFile) {
		std::cout << "Cannot open file." << std::endl;
		nodes = NULL;
		list = NULL;
		*m = -1;
		*n = -1;
		*totalCnodes = -1;
		return;
	}

	std::getline(nodeFile, line);
	std::istringstream iss(line);

	if(!(iss >> *m >> *n)) {
		std::cout << "Error: Cannot get matrix node dimensions." <<std::endl;
		nodes = NULL;
		list = NULL;
		*m = -1;
		*n = -1;
		*totalCnodes = -1;
		return;
	}

	std::cout << "Node Matrix Dimensions: " << *m << " x " << *n << std::endl;
	nodes = new Node[(*m+2)*(*n+2)];
	Coord* tempList = new Coord[(*m+2)*(*n+2)];

	*totalCnodes = 0;

	cl_int x,y;
	cl_char type;
	cl_float t;

	while (std::getline(nodeFile, line))
	{
		std::istringstream fileline(line);

		if(!(fileline >> x >> y >> type >> t))
			std::cout << "Error: Cannot retrieve node line."<<std::endl;

		nodes[x + y*(*n+1)].T = t + 273.15; // Convert to Kelvin
		nodes[x + y*(*n+1)].type = type;

		if (nodes[x + y*(*n+1)].type == 'c'){
			tempList[*totalCnodes].x = x;
			tempList[*totalCnodes].y = y;
			(*totalCnodes)++;
		}
	}

	nodeFile.close();

	list = new Coord[*totalCnodes];
	memcpy(list, tempList, sizeof(Coord)*(*totalCnodes));


	delete []tempList;
}


// Receiving full list of conduction nodes, split and divide the large list
// into smaller lists for every work item. Each work item will take a small list
// from an array of lists and the passed in nodes grid.



// After receiving all coords of conduction nodes, split the list equally into the
// number of work items. Work groups can share cache of old temperatures.


//Read lines of the file in parallel, add the each of the smaller lists at the same time
int main() {

	//buildKernel();

	double stability = C*dx*dx/(4*K);

	if (!stability_check(stability, dt))
		return -1;



	Node* nodes;

	Coord* list;

	int m,n, totalCnodes;

	readNodes("nodes.txt", nodes, list, &m, &n, &totalCnodes);

	Node* newnodes = new Node[(m+2)*(n+2)];

	//std::cout << nodes[1 + 3*(n+1)].T - 273.15 << std::endl;

//	std::cout <<"x: ";
//	for (int i=0; i < totalCnodes; ++i){
//		std::cout << list[i].x << " ";
//	}
//	std::cout << std::endl << "y: ";
//	for (int i=0; i < totalCnodes; ++i){
//			std::cout << list[i].y << " ";
//	}
//	std::cout << std::endl;



	//~~~~~~~~~~~~~~~~~~~~~~~~~~Initialize~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	    //Find all platforms installed on this computer
	    std::vector<cl::Platform> platforms;
	    cl::Platform::get(&platforms);
	    //std::cerr << "Platform number is: " << platforms.size() << std::endl;std::string platformVendor;

	    // Get Intel OpenCL platform
	    cl::Platform platform = platforms.front();
	    std::vector<cl::Device> devices;
	    platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);


	    // Get the GPU of the computer
	    auto device = devices.front();

	    std::ifstream GPUfile("thermoNode.cl");
	    checkErr(GPUfile.is_open() ? CL_SUCCESS:-1, "thermoNode.cl");
	    std::string src((std::istreambuf_iterator<char>(GPUfile)), (std::istreambuf_iterator<char>()));
	    cl::Program::Sources sources(1, std::make_pair(src.c_str(), src.length()+1));

	    cl::Context context(device);
	    cl::Program program(context, sources);

	    cl_int err = program.build("-cl-std=CL1.2");
	    std::cout << "Build: " <<(err == CL_SUCCESS) << std::endl;

	    cl::Buffer nodeBuf(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, sizeof(Node)*(m+2)*(n+2),nodes);//CL_MEM_COPY_HOST_PTR
	    cl::Buffer listBuf(context,CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, sizeof(Coord)*totalCnodes,list);
	    cl::Buffer newNodeBuf(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, sizeof(Node)*(m+2)*(n+2),newnodes);


	    cl::Kernel kernel(program,"thermoNode");
	    err = kernel.setArg(0,dt);
	    std::cout << "dt: " <<(err == CL_SUCCESS) << "\n";
	    err = kernel.setArg(1,dx);
	    std::cout << "dx: " << (err == CL_SUCCESS) << "\n";
	    err = kernel.setArg(2,K);
	    std::cout << "K: " <<(err == CL_SUCCESS) << "\n";
	    err = kernel.setArg(3,H);
	    std::cout << "H: " <<(err == CL_SUCCESS) << "\n";
	    err = kernel.setArg(4,C);
	    std::cout << "C: " <<(err == CL_SUCCESS) << "\n";
	    err = kernel.setArg(5,(cl_int)n);
	    std::cout << "Columns: " <<(err == CL_SUCCESS) << "\n";
	    err = kernel.setArg(6,totalCnodes);
	    std::cout << "TotalCNodes: " <<(err == CL_SUCCESS) << "\n";
	    err = kernel.setArg(7, nodeBuf);
	    std::cout << "Nodes: " << (err == CL_SUCCESS) << "\n";
	    err = kernel.setArg(8, newNodeBuf);
	    std::cout << "NewNodes: " << (err == CL_SUCCESS) << "\n";
		err = kernel.setArg(9,listBuf);
		std::cout << "Coords: " << (err == CL_SUCCESS) << "\n";

	    cl::CommandQueue queue(context, device);
	    //err = queue.enqueueFillBuffer(memBuf, 1,0, sizeof(Particle)*nparticle); //????
	    //std::cout << (err == CL_SUCCESS) <<"\n";
	    double elapsed_secs = 0;

	    std::clock_t begin, end;
	    std::ofstream output;
	    begin = std::clock();
	    if (std::ifstream("output.txt"))
	    	remove( "output.txt" );

	    output.open("output.txt");

	    printf("start\n");

	    int x,y,c;

	    c=count;
	    for (int i=0; i < iterations; ++i){
	    	err = queue.enqueueNDRangeKernel(kernel,cl::NullRange, cl::NDRange(workers), cl::NDRange(groupSize,groupSize));
	    	if (err != CL_SUCCESS)
	    		std::cout << "Enqueue: " << err << std::endl;
	    	err = queue.enqueueReadBuffer(nodeBuf, CL_TRUE, 0, sizeof(Node)*(m+2)*(n+2), nodes);

	    	//err = queue.enqueueReadBuffer(tempNewData, CL_TRUE, 0, sizeof(NewData)*totalCnodes, &tempStore);
	    	c--;

	    	if(c==0){
	    		c=count;
	    		for (y=1; y <= m; y++){
					for(x=1; x <= n; x++){
						output << nodes[x + y*(n+1)].T - 273.15 << " ";
					}
				}
				output<< std::endl;
	    	}


	    	//std::cout << nodes[x][y].T - 273.15 << std::endl;
//	    	if (err != CL_SUCCESS)
//	     		std::cout << "Read Buffer: " << err << std::endl;
	    }
	    output.close();


	    //queue.enqueueReadBuffer(outBuf, CL_TRUE, 0, sizeof(Particle)*nparticle, &particles);



	//    	for (int i = 0; i<nparticle; i++){
	//    		//std::cout << i << "test: " << particles[i].test  << '\n';
	//    		std::cout << i << "x: " << (particles[i].pos_new).x  << '\n';
	//    		std::cout << i << "y: " << (particles[i].pos_new).y  << '\n';
	//    		std::cout << i << "z: " << (particles[i].pos_new).z  << '\n';
	//    	}


	queue.finish();

	end = std::clock();
	elapsed_secs += (end - begin) / CLOCKS_PER_SEC;
	std::cout << "CPU+GPU Time: " << elapsed_secs << std::endl;

	delete []nodes;
	delete []list;
	delete []newnodes;
	//delete []tempStore;

	return 0;
}

//
//
//
//#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
//#include <iostream>
//#include <CL\cl.hpp>
//#include <CL\opencl.h>
//#include <fstream>
//#include <streambuf>
//#include <string>
//#include <sstream>
//#include <ctime>
//
//int main() {
//		const int m = 50;
//		const int n = 50;
//		const float init_t = 20;
//		const float top_t = 0;
//		const float left_t = 0;
//		const float right_t = 75;
//		const float bot_t = 50;
//		const char top = 's';
//		const char bot = 'c';
//		const char left = 's';
//		const char right = 'c';
//
//
//	    std::ofstream output;
//
//	    if (std::ifstream("nodes.txt"))
//	    	remove( "nodes.txt" );
//
//	    output.open("nodes.txt");
//
//	    output << m <<" "<<n<<std::endl;
//
//	    for(int i=1; i<(m+1);++i){
//
//	    	if (left == 's'){ // left side surrounding
//	    		output << "0 " << i << " " << "s " << left_t << " "<<std::endl;
//
//	    		if (i == 1 && bot == 'c'){ // bot/top constant temp
//	    			output << "1 " << i << " " << "t " << bot_t << " "<<std::endl;
//	    		}else if (i == m && top == 'c'){
//	    			output << "1 " << i << " " << "t " << top_t << " "<<std::endl;
//	    		} else{ // bot/top surrounding
//	    			output << "1 " << i << " " << "c " << init_t << " "<<std::endl;
//	    			if (i == 1)
//	    				output << "1 " << 0 << " " << "s " << bot_t << " "<<std::endl;
//	    			else if (i == m)
//	    				output << "1 " << i+1 << " " << "s " << top_t << " "<<std::endl;
//	    		}
//	    	}else{ // left side constant temp
//	    		if (i == 1 && bot == 'c'){
//	    			output << "1 " << i << " " << "t " << (left_t+bot_t)/2 << " "<<std::endl;
//	    		}else if (i == m && top == 'c'){
//	    			output << "1 " << i << " " << "t " << (left_t+top_t)/2 << " "<<std::endl;
//	    		} else{
//	    			output << "1 " << i << " " << "t " << left_t << " "<<std::endl;
//	    		}
//	    	}
//
//	    	int count = 0;
//
//	    	while (n - 2 > count){
//	    		if (i == 1 && bot == 'c')
//	    			output << (count+2) <<" " << i << " " << "t " << bot_t << " "<<std::endl;
//	    		else if ( i == m && top == 'c')
//	    			output << (count+2) <<" " << i << " " << "t " << top_t << " "<<std::endl;
//	    		else{
//	    			output << (count+2) <<" " << i << " " << "c " << init_t << " "<<std::endl;
//	    			if (i == 1)
//	    				output << (count+2) <<" " << 0 << " " << "t " << bot_t << " "<<std::endl;
//	    			else if (i == m)
//	    				output << (count+2) <<" " << i+1 << " " << "t " << top_t << " "<<std::endl;
//	    		}
//
//
//	    		count++;
//	    	}
//
//	    	if (right == 's'){ // right side surrounding
//	    		output << n + 1 << " " << i << " " << "s " << right_t << " "<<std::endl;
//
//	    		if (i == 1 && bot == 'c'){ // bot/top constant temp
//	    			output << n << " " << i << " " << "t " << bot_t << " "<<std::endl;
//	    		}else if (i == m && top == 'c'){
//	    			output << n << " " << i << " " << "t " << top_t << " "<<std::endl;
//	    		} else{ // bot/top surrounding
//	    			output << n << " " << i << " " << "c " << init_t << " "<<std::endl;
//	    			if (i == 1)
//	    				output << n << " " << 0 << " " << "s " << bot_t << " "<<std::endl;
//	    			else if (i == m)
//	    				output << n << " " << i+1 << " " << "s " << top_t << " "<<std::endl;
//	    		}
//	    	}else{ // right side constant temp
//	    		if (i == 1 && bot == 'c'){
//	    			output << n << " " << i << " " << "t " << (right_t+bot_t)/2 << " "<<std::endl;
//	    		}else if (i == m && top == 'c'){
//	    			output << n << " " << i << " " << "t " << (right_t+top_t)/2 << " "<<std::endl;
//	    		} else{
//	    			output << n << " " << i << " " << "t " << right_t << " "<<std::endl;
//	    		}
//	    	}
//
//	    	std::cout << std::endl;
//	    }
//
//
//	    output.close();
//
//	return 0;
//}
