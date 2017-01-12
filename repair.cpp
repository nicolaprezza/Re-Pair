/*
 * repair.cpp
 *
 *  Created on: Jan 11, 2017
 *      Author: nico
 */

#include <chrono>
#include <string>
#include <iostream>
#include <vector>

#include <hf_queue.hpp>
#include <ll_vec.hpp>
#include <ll_el.hpp>

using namespace std;

void help(){

	cout << "Compute space-efficiently (roughly (2n + sqrt(n)) * log n bits) the Re-Pair grammar of a string" << endl << endl;
	cout << "Usage: repair <input> <output>" << endl;
	cout << "   <input>   input text file" << endl;
	cout << "   <output>  two files are generated:" << endl;
	cout << "             1. output.rp is the compressed text composed of dictionary/alphabet " << endl;
	cout << "                symbols (integers) such that no pair of adjacent symbols appears more than once." << endl;
	cout << "             2. output.g contains the Re-Pair grammar. This is a binary file composed of a" << endl;
	cout << "                list of triples (integers) XAB representing rule X -> AB" << endl;
	exit(0);

}


int main(int argc,char** argv) {

	if(argc!=3) help();

	string in(argv[1]);
	string out_prefix(argv[2]);

	string out_rp = out_prefix;
	out_rp.append(".rp");

	string out_g = out_prefix;
	out_g.append(".g");

	cout << "Compressing file " << in << endl;
	cout << "Saving output to files " << out_rp  << " and " << out_g << endl<<endl;

	hf_queue32_t Q(5,9);




}


