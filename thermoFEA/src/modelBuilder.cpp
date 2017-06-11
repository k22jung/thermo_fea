#include "modelBuilder.hpp"

#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#include <iostream>
#include <fstream>
#include <streambuf>
#include <string>
#include <sstream>

const int m = 100;
const int n = 100;
const float init_t = 0;
const float top_t = 100;
const float left_t = 0;
const float right_t = 100;
const float bot_t = 0;
const char top = 'c';
const char bot = 'c';
const char left = 'c';
const char right = 'c';


// Builds a square/rectangular plate of nodes based on the above parameters
void createNodes() {

	    std::ofstream output;

	    if (std::ifstream("nodes.txt"))
	    	remove( "nodes.txt" );

	    output.open("nodes.txt");

	    output << m <<" "<<n<<std::endl;

	    for(int i=1; i<(m+1);++i){

	    	if (left == 's'){ // left side surrounding
	    		output << "0 " << i << " " << "s " << left_t << " "<<std::endl;

	    		if (i == 1 && bot == 'c'){ // bot/top constant temp
	    			output << "1 " << i << " " << "t " << bot_t << " "<<std::endl;
	    		}else if (i == m && top == 'c'){
	    			output << "1 " << i << " " << "t " << top_t << " "<<std::endl;
	    		} else{ // bot/top surrounding
	    			output << "1 " << i << " " << "c " << init_t << " "<<std::endl;
	    			if (i == 1)
	    				output << "1 " << 0 << " " << "s " << bot_t << " "<<std::endl;
	    			else if (i == m)
	    				output << "1 " << i+1 << " " << "s " << top_t << " "<<std::endl;
	    		}
	    	}else{ // left side constant temp
	    		if (i == 1 && bot == 'c'){
	    			output << "1 " << i << " " << "t " << (left_t+bot_t)/2 << " "<<std::endl;
	    		}else if (i == m && top == 'c'){
	    			output << "1 " << i << " " << "t " << (left_t+top_t)/2 << " "<<std::endl;
	    		} else{
	    			output << "1 " << i << " " << "t " << left_t << " "<<std::endl;
	    		}
	    	}

	    	int count = 0;

	    	while (n - 2 > count){
	    		if (i == 1 && bot == 'c')
	    			output << (count+2) <<" " << i << " " << "t " << bot_t << " "<<std::endl;
	    		else if ( i == m && top == 'c')
	    			output << (count+2) <<" " << i << " " << "t " << top_t << " "<<std::endl;
	    		else{
	    			output << (count+2) <<" " << i << " " << "c " << init_t << " "<<std::endl;
	    			if (i == 1)
	    				output << (count+2) <<" " << 0 << " " << "t " << bot_t << " "<<std::endl;
	    			else if (i == m)
	    				output << (count+2) <<" " << i+1 << " " << "t " << top_t << " "<<std::endl;
	    		}


	    		count++;
	    	}

	    	if (right == 's'){ // right side surrounding
	    		output << n + 1 << " " << i << " " << "s " << right_t << " "<<std::endl;

	    		if (i == 1 && bot == 'c'){ // bot/top constant temp
	    			output << n << " " << i << " " << "t " << bot_t << " "<<std::endl;
	    		}else if (i == m && top == 'c'){
	    			output << n << " " << i << " " << "t " << top_t << " "<<std::endl;
	    		} else{ // bot/top surrounding
	    			output << n << " " << i << " " << "c " << init_t << " "<<std::endl;
	    			if (i == 1)
	    				output << n << " " << 0 << " " << "s " << bot_t << " "<<std::endl;
	    			else if (i == m)
	    				output << n << " " << i+1 << " " << "s " << top_t << " "<<std::endl;
	    		}
	    	}else{ // right side constant temp
	    		if (i == 1 && bot == 'c'){
	    			output << n << " " << i << " " << "t " << (right_t+bot_t)/2 << " "<<std::endl;
	    		}else if (i == m && top == 'c'){
	    			output << n << " " << i << " " << "t " << (right_t+top_t)/2 << " "<<std::endl;
	    		} else{
	    			output << n << " " << i << " " << "t " << right_t << " "<<std::endl;
	    		}
	    	}

	    	std::cout << std::endl;
	    }
	    output.close();
}
