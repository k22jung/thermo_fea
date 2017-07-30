# Project Title

**Thermo GPU** is another small project I completed which utilizes a computer's GPU to parallelize computation for a large thermodynamic Finite Element Methods problem, consisting of thousands of nodes. 

The objective of the project was to dramatically decrease runtime in comparison to the implementation found from MathWork's [Webinar](https://www.mathworks.com/videos/teaching-fluid-mechanics-and-heat-transfer-with-interactive-matlab-apps-81962.html). This was successfully accomplished, decreasing the runtime from the above MATLAB by over 900 times, using OpenCL and parallelizing repetitive calculations using my computer's GPU. I wrote a kernel to concurrently execute the program in parallel over thousands of GPU work-items.

To visualize the output from the OpenCL project, I wrote a MATLAB script to output every time step's iteration to a figure. The following image is an example of the steady-state thermodynamic solution of a long aluminum bar's square cross-section. The boundary conditions are constant temperatures applied to each side of the long metal bar. The axes plot the node numbers in the x and y directions.


<p align="center"> 
<img src="https://github.com/k22jung/thermo_gpu/blob/master/thermoDisplay/steady-state.png">
</p>


## Dependencies

- [Intel's OpenCL 1.2](https://software.intel.com/en-us/intel-opencl)
- 

## Running
