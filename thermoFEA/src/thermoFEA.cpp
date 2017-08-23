/*
* Kenneth Jung
* Mechatronics Engineering
* k22jung@edu.uwaterloo.ca
*/
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

using namespace std;
using namespace cl;

#pragma pack(16)
struct Node {
    cl_float T;
    cl_char type; // c = conduction, g = ghost node (used for insulated points), t = constant temperature, s = surrounding temp (convection)
};

struct Coord {
    cl_int x;
    cl_int y;
};
#pragma pack(16)


inline void checkErr(cl_int err, const char * name) {
	if (err != CL_SUCCESS) {
	cerr << "ERROR: " << name  << " (" << err << ")" << endl;
	exit(EXIT_FAILURE);
	}
}

// ~~~~~~~~~~ Constants ~~~~~~~~~~~
/* Resources on materials:
 * http://www.engineeringtoolbox.com/metal-alloys-densities-d_50.html
 * http://www.engineersedge.com/materials/specific_heat_capacity_of_metals_13259.htm
 * https://en.wikipedia.org/wiki/Volumetric_heat_capacity
 */
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
const cl_float C = Density*Cspec;

const char dirOut[] = "<directory>\\output.txt";
const char kernelName[] = "thermoNode.cl";

bool stability_check (double stability, const cl_float dt, int totalCnodes){
	if (stability <= dt){
		cout << "Parameters do not guarantee stability (" << stability << " <= " << dt << ")" << endl;
		return false;
	}else if(workers%groupSize != 0){
		cout << "Workers must be equally divisible by group size." <<endl;
		return false;
	}else if (totalCnodes < workers){
		cout << "Too many threads created." << endl;
		return false;
	}
	return true;
}

void readNodes(string filename, Node* &nodes, Node* &newnodes, Coord* &list, int* m, int* n, int* totalCnodes) {
	string line;
	ifstream nodeFile(filename);

	if(!nodeFile) {
		cout << "Cannot open file." << endl;
		nodes = NULL;
		list = NULL;
		*m = -1;
		*n = -1;
		*totalCnodes = -1;
		return;
	}

	getline(nodeFile, line);
	istringstream iss(line);

	if(!(iss >> *m >> *n)) {
		cout << "Error: Cannot get matrix node dimensions." <<endl;
		nodes = NULL;
		list = NULL;
		*m = -1;
		*n = -1;
		*totalCnodes = -1;
		return;
	}

	cout << "Node Matrix Dimensions: " << *m << " x " << *n << endl;
	nodes = new Node[(*m+2)*(*n+2)];
	newnodes = new Node[(*m+2)*(*n+2)];

	Coord* tempList = new Coord[(*m+2)*(*n+2)];

	*totalCnodes = 0;

	cl_int x,y;
	cl_char type;
	cl_float t;

	while (getline(nodeFile, line))
	{
		istringstream fileline(line);

		if(!(fileline >> x >> y >> type >> t))
			cout << "Error: Cannot retrieve node line."<<endl;

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

void writeFile(CommandQueue &queue, Buffer &nodeBuf, Node* &nodes, ofstream &output, int m, int n){
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

Program::Sources getKernelSource(){
	ifstream kernelfile(kernelName);
	checkErr(kernelfile.is_open() ? CL_SUCCESS:-1, kernelName);
	string src((istreambuf_iterator<char>(kernelfile)), (istreambuf_iterator<char>()));
	Program::Sources sources(1, make_pair(src.c_str(), src.length()+1));

	return sources;
}

Device getGPU (){
	vector<Platform> platforms;
	Platform::get(&platforms);
	Platform platform = platforms.front();

	vector<Device> devices;
	platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

	// Return the GPU of the computer
	return devices.front();
}



int main() {

	createNodes();

	double stability = C*dx*dx/(4*K);

	Node* nodes;
	Node* newnodes;
	Coord* list;
	int m,n,totalCnodes;

	readNodes("nodes.txt", nodes, newnodes,list, &m, &n, &totalCnodes);

	cout << "Processes per FEM Iteration: " << totalCnodes << endl;

	if (!stability_check(stability, dt, totalCnodes)){
		delete []nodes;
		delete []newnodes;
		delete []list;
		return -1;
	}

	Device device = getGPU();
	Context context(device);
	Program program(context, getKernelSource());

	program.build("-cl-std=CL1.2");

	Buffer nodeBuf(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, sizeof(Node)*(m+2)*(n+2),nodes);//CL_MEM_COPY_HOST_PTR
	Buffer listBuf(context,CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, sizeof(Coord)*totalCnodes,list);
	Buffer newNodeBuf(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, sizeof(Node)*(m+2)*(n+2),newnodes);

	Kernel kernel(program,"thermoNode");
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

	CommandQueue queue(context, device);
	double elapsed_secs = 0;
	double elapsed_file = 0;

	clock_t begin, end;

	ofstream output;
	if (ifstream(dirOut))
		remove(dirOut);
	output.open(dirOut);

	cl_int err;
	int c = 0;

	cout << "\nBegin computing...\n" << endl;
	for (int i=0; i < iterations; ++i){

		// ~~~~~~~~~~~~ Enqueue computation commands ~~~~~~~~~~~~
		begin = clock();
		err = queue.enqueueNDRangeKernel(kernel,NullRange, NDRange(workers), NDRange(groupSize,groupSize));
		queue.finish();
		end = clock();

		elapsed_secs += (end - begin)/ (double)CLOCKS_PER_SEC;

		if (err != CL_SUCCESS)
			cout << "Enqueue: " << err << endl;
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


		// ~~~~~~~~~~~~ Read to Host Memory and Write Out File ~~~~~~~~~~~~
		/*
		 * If the current iteration is meant for file writing, the program will
		 * copy temperature data from the kernel buffer to the host memory and
		 * output this data into a text file.
		 */
		begin = clock();

		c++;

		if(c == count){
			c = 0;

			if(i%2 == 0)
				writeFile(queue,nodeBuf,nodes,output, m, n);
			else
				writeFile(queue,newNodeBuf,newnodes,output, m, n);
		}
		end = clock();
		elapsed_file += (end - begin)/ (double)CLOCKS_PER_SEC;
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		/*
		 * The kernel uses two large buffers to store the old and new
		 * temperatures respectively of every iteration, nodeBuf and
		 * newNodeBuf. The kernel will alternate these buffers as kernel
		 * arguments so newNodeBuf data won't need to waste time copying
		 * back to nodeBuf required for the next iteration to use.
		 */
		if (i%2 == 0){
			err = kernel.setArg(8, nodeBuf);
			err = kernel.setArg(7, newNodeBuf);
		}else{
			err = kernel.setArg(7, nodeBuf);
			err = kernel.setArg(8, newNodeBuf);
		}

	}
	output.close();

	cout << "GPU Compute Time: " << elapsed_secs << "s" << endl;
	cout << "GPU Read + File Write Time: " << elapsed_file << "s" << endl;
	cout << "Read/Write time Percent: " << elapsed_file*100/(elapsed_secs+elapsed_file) << " %" << endl;

	delete []nodes;
	delete []list;
	delete []newnodes;

	return 0;
}

