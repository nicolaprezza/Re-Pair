/*
 *  This file is part of Re-Pair.
 *  Copyright (c) by
 *  Nicola Prezza <nicola.prezza@gmail.com>, Philip Bille, and Inge Li Gørtz
 *
 *   Re-Pair is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.

 *   Re-Pair is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details (<http://www.gnu.org/licenses/>).
 *
 *
 * repair.cpp
 *
 *  Created on: Jan 11, 2017
 *      Author: nico
 */

#include <chrono>
#include <string>
#include <iostream>
#include <vector>
#include <set>

#include <lf_queue.hpp>

#include <ll_vec.hpp>
#include <ll_el.hpp>

#include <fstream>
#include <cmath>
#include <stack>

#include "internal/packed_gamma_file.hpp"
#include "internal/packed_gamma_file2.hpp"
#include "internal/packed_gamma_file3.hpp"

using namespace std;

using itype = uint32_t;

void help(){

	cout << "Decompress a file created with repair-compress." << endl << endl;
	cout << "Usage: repair-decompress <input> [output]" << endl;
	cout << "   <input>   input archive" << endl;
	cout << "   [output]  optional output file name. If not specified, output is saved to input.decompressed" << endl;
	exit(0);

}

void decompress(vector<itype> & A, vector<pair<itype,itype> > & G, vector<itype> & Tc, ofstream & ofs){

	std::stack<itype> S;

	string buffer;
	int buf_size = 1000000;//1 MB buffer

	/*
	 * decompress Tc symbols one by one
	 */
	for(itype i = 0;i<Tc.size();++i){

		S.push(Tc[i]);

		while(!S.empty()){

			itype X = S.top(); //get symbol
			S.pop();//remove top

			if(X<A.size()){

				char c = A[X];

				buffer.push_back(c);

				if(buffer.size()==buf_size){

					ofs.write(buffer.c_str(),buffer.size());
					buffer = string();

				}

			}else{

				//expand rule: X -> ab
				auto ab = G[X-A.size()];

				S.push(ab.second);
				S.push(ab.first);

			}

		}

	}

	if(buffer.size()>0) ofs.write(buffer.c_str(),buffer.size());

}

int main(int argc,char** argv) {

	if(argc!=3 and argc != 2) help();

	string in(argv[1]);

	string out;

	if(argc == 3){

		out = string(argv[2]);

	}else{

		out = string(argv[1]);
		out.append(".decompressed");

	}

	if(not ifstream(in).good()) help();

	cout << "Decompressing archive " << in << endl;
	cout << "Output will be saved to " << out << endl;

	auto pgf = packed_gamma_file3<>(in, false);
	//auto pgf = packed_gamma_file2<>(in, false);
	//auto pgf = packed_gamma_file<>(in, false);

	vector<itype> A;
	vector<pair<itype,itype> > G;
	vector<itype> Tc;

	//read and decompress grammar
	pgf.read_and_decompress(A,G,Tc);

	ofstream ofs(out);

	decompress(A,G,Tc,ofs);

	ofs.close();

	cout << "done." << endl;

}


