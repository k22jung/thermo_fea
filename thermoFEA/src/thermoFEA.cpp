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
#include "modelBuilder.hpp"
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

#pragma pack(16)

//CL_INVALID_WORK_ITEM_SIZE;
const int workers = 9500;
const int groupSize = 50;
const cl_float K = 205;
const cl_float dx = 0.001;
const cl_float dt = 0.001;
const cl_float dt_display = 0.01;
const int count = dt_display/dt;
const cl_float H = 1000;
const cl_float totalTime = 10;
const int iterations = totalTime/dt;
const cl_float Density = 2720;
const cl_float Cspec = 921.096;
//http://www.engineeringtoolbox.com/metal-alloys-densities-d_50.html
//http://www.engineersedge.com/materials/specific_heat_capacity_of_metals_13259.htm
//https://en.wikipedia.org/wiki/Volumetric_heat_capacity
const cl_float C = Density*Cspec;

const char dirOut[] = "C:\\Users\\Kenny\\Desktop\\Side Projects\\Current Projects\\thermoGPU\\thermoDisplay\\output.txt";


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
	}else if(workers%groupSize!=0){
		std::cout << "Workers must be equally divisible by group size." <<std::endl;
		return false;
	}
	return true;
}

void readNodes(std::string filename, Node* &nodes, Node* &newnodes, Coord* &list, int* m, int* n, int* totalCnodes) {
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
	newnodes = new Node[(*m+2)*(*n+2)];

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
		newnodes[x + y*(*n+1)].T = t + 273.15; // Convert to Kelvin
		newnodes[x + y*(*n+1)].type = type;

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

void writeFile(cl::CommandQueue &queue,cl::Buffer &nodeBuf,Node* &nodes,std::ofstream &output, int m, int n){
	queue.enqueueReadBuffer(nodeBuf, CL_TRUE, 0, sizeof(Node)*(m+2)*(n+2), nodes);
	queue.finish();

	int x,y;

	for (y=1; y <= m; y++){
		for(x=1; x <= n; x++){
			output << nodes[x + y*(n+1)].T - 273.15 << ' ';
		}
	}
	output<< '\n';
}
int main() {

	//buildKernel();
	createNodes();

	double stability = C*dx*dx/(4*K);

	if (!stability_check(stability, dt))
		return -1;



	Node* nodes;
	Node* newnodes;

	Coord* list;

	int m,n,totalCnodes;

	readNodes("nodes.txt", nodes, newnodes,list, &m, &n, &totalCnodes);

	std::cout << "totalCnodes: " << totalCnodes << std::endl;
	if (totalCnodes < workers){
		delete []nodes;
		delete []list;
		std::cout << "Too many threads created." << std::endl;
		return -1;
	}



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
	    kernel.setArg(0,dt);
	    kernel.setArg(1,dx);
	    kernel.setArg(2,K);
	    kernel.setArg(3,H);
	    kernel.setArg(4,C);
	    kernel.setArg(5,(cl_int)n);
	    kernel.setArg(6,totalCnodes);
	    kernel.setArg(7,nodeBuf);
	    kernel.setArg(8,newNodeBuf);
		kernel.setArg(9,listBuf);

	    cl::CommandQueue queue(context, device);
	    double elapsed_secs = 0;
	    double elapsed_file = 0;

	    std::clock_t begin, end;
	    std::ofstream output;
	    begin = std::clock();

	    if (std::ifstream(dirOut))
	    	remove(dirOut);

	    output.open(dirOut);

	    int c = 0;

	    for (int i=0; i < iterations; ++i){

	    	begin = std::clock();
	    	err = queue.enqueueNDRangeKernel(kernel,cl::NullRange, cl::NDRange(workers), cl::NDRange(groupSize,groupSize));
	    	queue.finish();
	    	end = std::clock();
	    	elapsed_secs += (end - begin)/ (double)CLOCKS_PER_SEC;

	    	if (err != CL_SUCCESS)
	    		std::cout << "Enqueue: " << err << std::endl;



	    	begin = std::clock();
	    	c++;
	    	if(c == count){
	    		c=0;

	    		if(i%2 == 0)
	    			writeFile(queue,nodeBuf,nodes,output, m, n);
	    		else
	    			writeFile(queue,newNodeBuf,newnodes,output, m, n);
	    	}
	    	end = std::clock();
	    	elapsed_file += (end - begin)/ (double)CLOCKS_PER_SEC;



	    	if (i%2 == 0){
	    		err = kernel.setArg(8, nodeBuf);
	    		err = kernel.setArg(7, newNodeBuf);
	    	}else{
	    		err = kernel.setArg(7, nodeBuf);
	    		err = kernel.setArg(8, newNodeBuf);
	    	}

	    }
	    output.close();

	std::cout << "GPU Compute Time: " << elapsed_secs << " s" << std::endl;
	std::cout << "GPU Read + File Write Time: " << elapsed_file << " s" << std::endl;
	std::cout << "Read/Write time Percent: " << elapsed_file*100/(elapsed_secs+elapsed_file) << " %" << std::endl;
	delete []nodes;
	delete []list;
	delete []newnodes;

	return 0;
}

