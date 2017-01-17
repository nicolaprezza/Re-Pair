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
#include <lf_queue.hpp>

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

	using triple = std::tuple<uint32_t, uint32_t,uint32_t>;

	if(argc!=3) help();

	string in(argv[1]);
	string out_prefix(argv[2]);

	string out_rp = out_prefix;
	out_rp.append(".rp");

	string out_g = out_prefix;
	out_g.append(".g");

	cout << "Compressing file " << in << endl;
	cout << "Saving output to files " << out_rp  << " and " << out_g << endl<<endl;

	lf_queue32_t Q(9,15);

	Q.push_back_frequency(4);
	Q.push_back_frequency(7);
	Q.push_back_frequency(9);
	Q.push_back_frequency(10);
	Q.push_back_frequency(12);

	lf_queue32_t::el_type e {{'a','b'},1,5,12};
	Q.insert(e);

	e = {{'c','b'},2,3,9};
	Q.insert(e);

	e = {{'x','x'},1,2,10};
	Q.insert(e);

	e = {{'a','v'},4,2,7};
	Q.insert(e);

	e = {{'s','b'},6,1,9};
	Q.insert(e);

	e = {{'a','e'},6,1,4};
	Q.insert(e);

	e = {{'a','m'},6,1,4};
	Q.insert(e);

	e = {{'m','m'},6,1,4};
	Q.insert(e);

	e = {{'n','m'},6,1,4};
	Q.insert(e);

	Q.remove({'a','m'});

	if(Q.contains({'a','e'})){

		hf_queue32_t::triple_t t = Q[{'a','e'}];
		cout << t.F_ab << endl;
		Q.decrease({'a','e'});

	}else{
		cout << "does not contain" << endl;
	}




	{
		hf_queue32_t::triple_t t = Q[{'a','e'}];
		cout << "F_ae = " << t.F_ab << endl;
	}

	if(Q.contains({'a','e'})){

		hf_queue32_t::triple_t t = Q[{'a','e'}];
		cout << t.F_ab << endl;
		Q.decrease({'a','e'});

	}else{
		cout << "does not contain" << endl;
	}

	{
		hf_queue32_t::triple_t t = Q[{'a','e'}];
		cout << "F_ae = " << t.F_ab << endl;
	}

	if(Q.contains({'a','e'})){

		hf_queue32_t::triple_t t = Q[{'a','e'}];
		cout << t.F_ab << endl;
		Q.decrease({'a','e'});

	}else{
		cout << "does not contain" << endl;
	}

	if(Q.contains({'a','e'})){

		hf_queue32_t::triple_t t = Q[{'a','e'}];
		cout << t.F_ab << endl;
		Q.decrease({'a','e'});

	}else{
		cout << "does not contain" << endl;
	}

	//Q.remove({'a','b'});

	{
		auto pp = Q.max();

		hf_queue32_t::triple_t t = Q[pp];
		cout << "max freq = " << t.F_ab << endl;

	}

	while(Q.size() > 0){

		auto pp = Q.max();

		cout << "extracted " << (char)pp.first << (char)pp.second << " " << flush;

		Q.remove(pp);

		cout << "size = " << Q.size() << endl;

	}





}


