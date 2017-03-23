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
 * rp.cpp
 *
 *  Created on: Jan 11, 2017
 *      Author: nico
 */

#include <chrono>
#include <string>
#include <iostream>
#include <vector>
#include <set>
#include <stack>

#include <lf_queue.hpp>

#include <ll_vec.hpp>
#include <ll_el.hpp>

#include <fstream>
#include <cmath>

#include "internal/hf_queue.hpp"
#include "internal/skippable_text.hpp"
#include "internal/text_positions.hpp"
#include "internal/packed_gamma_file3.hpp"

using namespace std;

void help(){

	cout << "Compressor and decompressor based on the Re-Pair grammar. Space usage: roughly 6n Bytes of RAM, where n < 2^32 is the file size." << endl << endl;
	cout << "Usage: rp <c|d> <input> [output]" << endl;
	cout << "   c         compress <input>" << endl;
	cout << "   d         decompress <input>" << endl;
	cout << "   <input>   input text file (compression mode) or rp archive (decompression mode)" << endl;
	cout << "   [output]  optional output file name. If not specified, suffix .rp is added (compression) or removed (decompression)" << endl;
	exit(0);

}

//high/low frequency text type
using text_t = skippable_text32_t;
using TP_t = text_positions32_t;
using hf_q_t = hf_queue32_t;
using lf_q_t = lf_queue32_t;
using itype = uint32_t;

/*
using text_t = skippable_text64_t;
using TP_t = text_positions64_t;
using hf_q_t = hf_queue64_t;
using lf_q_t = lf_queue64_t;
using itype = uint64_t;
*/

using cpair = hf_q_t::cpair;

//next free dictionary symbol
itype X=0;
itype last_freq = 0;
itype n_distinct_freqs = 0;

vector<itype> A; //alphabet (mapping int->ascii)
vector<pair<itype, itype> > G; //grammar
vector<itype> T_vec;// compressed text

/*
 * Given (empty) queue, text positions, text, and minimum frequency: insert in Q all pairs with frequency at least min_freq.
 *
 * assumptions: TP is sorted by character pairs, Q is void
 *
 */
void new_high_frequency_queue(hf_q_t & Q, TP_t & TP, text_t & T, uint64_t min_freq){

	itype j = 0; //current position on TP
	itype n = TP.size();

	int old_perc = 0;
	int perc;

	itype n_pairs = 0;

	/*
	 * step 1: count number of high-freq pairs
	 */
	while(j<n){

		itype k = 1; //current pair frequency

		while(	j<TP.size()-1 &&
				T.pair_starting_at(TP[j]) != T.blank_pair() &&
				T.pair_starting_at(TP[j]) == T.pair_starting_at(TP[j+1]) ){

			j++;
			k++;

		}

		if(k>=min_freq){

			n_pairs++;

		}

		j++;

	}

	//largest possible dictionary symbol
	itype max_d = 256+T.size()/min_freq;

	//create new queue. Capacity is number of pairs / min_frequency
	Q.init(max_d,min_freq);

	/*
	 * step 2. Fill queue
	 */
	j = 0;
	while(j<n){

		itype P_ab = j; //starting position in TP of pair

		itype k = 1; //current pair frequency
		cpair ab = T.blank_pair();

		while(	j<TP.size()-1 &&
				T.pair_starting_at(TP[j]) != T.blank_pair() &&
				T.pair_starting_at(TP[j]) == T.pair_starting_at(TP[j+1]) ){

			ab = T.pair_starting_at(TP[j]);

			j++;
			k++;

		}

		if(k>=min_freq){

			Q.insert({ab, P_ab, k, k});

		}

		j++;

	}

}


/*
 * synchronize queue in range corresponding to pair AB.
 */
template<typename queue_t>
void synchronize(queue_t & Q, TP_t & TP, text_t & T, cpair AB){

	//variables associated with AB
	assert(Q.contains(AB));
	auto q_el = Q[AB];
	itype P_AB = q_el.P_ab;
	itype L_AB = q_el.L_ab;
	itype F_AB = q_el.F_ab;

	itype freq_AB = 0;//number of pairs AB seen inside the interval. Computed inside this function

	assert(P_AB+L_AB <= TP.size());
	//sort sub-array corresponding to AB
	TP.cluster(P_AB,P_AB+L_AB);
	assert(TP.is_clustered(P_AB,P_AB+L_AB));

	//scan TP[P_AB,...,P_AB+L_AB-1] and detect new pairs
	itype j = P_AB;//current position in TP
	while(j<P_AB+L_AB){

		itype p = j; //starting position of current pair in TP
		itype k = 1; //current pair frequency

		cpair XY = T.pair_starting_at(TP[j]);

		while(	j<(P_AB+L_AB)-1 &&
				XY != T.blank_pair() &&
				XY == T.pair_starting_at(TP[j+1]) ){

			j++;
			k++;

		}

		freq_AB = XY == AB ? k : freq_AB;

		if(k >= Q.minimum_frequency()){

			//if the pair is not AB and it is a high-frequency pair, insert it in queue
			if(XY != AB){

				assert(XY != T.blank_pair());

				assert(not Q.contains(XY));

				Q.insert({XY,p,k,k});

				assert(TP.contains_only(p,p+k,XY));

			}else if(XY == AB){ //the pair is AB and is already in the queue: update its frequency

				assert(Q.contains(AB));
				Q.update({AB,p,k,k});

				assert(TP.contains_only(p,p+k,AB));

			}

		}

		j++;

	}

	assert(Q.contains(AB));

	//it could be that now AB's frequency is too small: delete it
	if(freq_AB < Q.minimum_frequency()){

		Q.remove(AB);

	}

	assert(not Q.contains(AB) || Q[AB].F_ab == Q[AB].L_ab);

}


/*
 * look at F_ab and L_ab. Cases:
 *
 * 1. F_ab <= L_ab/2 and F_ab >= min_freq: synchronize pair. There could be new high-freq pairs in ab's list
 * 2. F_ab <= L_ab/2 and F_ab < min_freq: as above. This because there could be new high-freq pairs in ab's list.
 * 3. F_ab > L_ab/2 and F_ab >= min_freq: do nothing
 * 4. F_ab > L_ab/2 and F_ab < min_freq: remove ab. ab's list cannot contain high-freq pairs, so it is safe to lose references to these pairs.
 *
 */
template<typename queue_t>
void synchro_or_remove_pair(queue_t & Q, TP_t & TP, text_t & T, cpair ab){

	assert(Q.contains(ab));

	auto q_el = Q[ab];
	itype F_ab = q_el.F_ab;
	itype L_ab = q_el.L_ab;

	if(F_ab <= L_ab/2){

		synchronize<queue_t>(Q, TP, T, ab);

	}else{

		if(F_ab < Q.minimum_frequency()){

			Q.remove(ab);

		}

	}

}


/*
 * bit-width of x
 */
uint64_t wd(uint64_t x){

	auto w = 64 - __builtin_clzll(uint64_t(x));

	return x == 0 ? 1 : w;

}


const pair<itype,itype> nullpair = {~itype(0),~itype(0)};


/*
 * return frequency of replaced pair
 */
template<typename queue_t>
uint64_t substitution_round(queue_t & Q, TP_t & TP, text_t & T){

	using ctype = text_t::char_type;

	//compute max
	cpair AB = Q.max();

	G.push_back(AB);

	//cout << "MAX freq = " << Q[AB].F_ab << endl;

	assert(Q.contains(AB));
	assert(Q[AB].F_ab >= Q.minimum_frequency());

	//extract P_AB and L_AB
	auto q_el = Q[AB];
	itype F_AB = q_el.F_ab;
	itype P_AB = q_el.P_ab;
	itype L_AB = q_el.L_ab;

	uint64_t f_replaced = F_AB;

	n_distinct_freqs += (F_AB != last_freq);
	last_freq = F_AB;

	for(itype j = P_AB; j<P_AB+L_AB;++j){

		itype i = TP[j];

		if(T.pair_starting_at(i) == AB){

			ctype A = AB.first;
			ctype B = AB.second;

			//the context of AB is xABy. We now extract AB's context:
			cpair xA = T.pair_ending_at(i);
			cpair By = T.next_pair(i);

			assert(xA == T.blank_pair() or xA.second == A);
			assert(By == T.blank_pair() or By.first == B);

			//note: xA and By could be blank pairs if this AB was the first/last pair in the text

			//perform replacement
			T.replace(i,X);

			assert(By == T.blank_pair() || T.pair_starting_at(i) == cpair(X,By.second));

			if(Q.contains(xA) && xA != AB){

				Q.decrease(xA);

			}

			if(Q.contains(By) && By != AB){

				Q.decrease(By);

			}

		}

	}

	/*
	 * re-scan text positions associated to AB and synchronize if needed
	 */
	for(itype j = P_AB; j<P_AB+L_AB;++j){

		itype i = TP[j];

		assert(T.pair_starting_at(i) != AB); //we replaced all ABs ...

		if(T[i] == X){

			//the context of X is xXy. We now extract X's left (x) and right (y) contexts:
			cpair xX = T.pair_ending_at(i);
			cpair Xy = T.pair_starting_at(i);

			ctype A = AB.first;
			ctype B = AB.second;

			//careful: x and y could be = X. in this case, before the replacements this xX was equal to ABAB -> a BA disappeared
			ctype x = xX.first == X ? B : xX.first;
			ctype y = Xy.second == X ? A : Xy.second;

			//these are the pairs that disappeared
			cpair xA = xX == T.blank_pair() ? xX : cpair {x,A};
			cpair By = Xy == T.blank_pair() ? Xy : cpair {B,y};

			if(Q.contains(By) && By != AB){

				synchro_or_remove_pair<queue_t>(Q, TP, T, By);

			}

			if(Q.contains(xA) && xA != AB){

				synchro_or_remove_pair<queue_t>(Q, TP, T, xA);

			}

		}

	}

	assert(Q.contains(AB));
	synchronize<queue_t>(Q, TP, T, AB); //automatically removes AB since new AB's frequency is 0
	assert(not Q.contains(AB));

	//advance next free dictionary symbol
	X++;

	//cout << " current text size = " << T.number_of_non_blank_characters() << endl << endl;

	return f_replaced;

}


void compute_repair(string in){


	//packed_gamma_file2<> out_file(out);
	//packed_gamma_file2<> out_file(out);

	/*
	 * tradeoff between low-frequency and high-freq phase:
	 *
	 * - High-freq phase will use n^(2 - 2*alpha) words of memory and process approximately n^(1-alpha) pairs
	 *   alpha should satisfy 0.5 < alpha < 1
	 *
	 * - Low-freq phase will use n^alpha words of memory
	 *
	 */
	double alpha = 0.66; // = 2/3

	/*
	 * in the low-frequency pair processing phase, insert at most n/B elements in the hash
	 */
	uint64_t B = 50;

	itype n;
	itype sigma = 0; //alphabet size

	/*
	 * Pairs with frequency greater than or equal to min_high_frequency are inserted in high-freq queue
	 */
	itype min_high_frequency = 0;
	itype lf_queue_capacity = 0;

	/*
	 * (1) INITIALIZE DATA STRUCTURES
	 */

	const itype null = ~itype(0);

	ifstream ifs(in);

	vector<itype> char_to_int(256,null);

	//count file size
	{
		ifstream file(in,ios::ate);
		n = file.tellg();
	}

	min_high_frequency = std::pow(  n, alpha  );// n^(alpha)

	min_high_frequency = min_high_frequency <2 ? 2 : min_high_frequency;

	cout << "File size = " << n << " characters"  << endl;
	cout << "cut-off frequency = " << min_high_frequency  << endl;

	itype width = 64 - __builtin_clzll(uint64_t(n));

	//largest possible high-frequency dictionary symbol
	//at most max_d <= n high-freq dictionary symbols can be created; given that min. freq of
	//a high-freq dictionary symbol is f=min_high_frequency and that every new dictionary symbol
	//introduces a blank in the text, we have the inequality max_d * f <= n  <=> max_d <= n/f
	itype max_d = 256+n/min_high_frequency;

	//cout << "Max high-frequency dictionary symbol = " << max_d << endl << endl;

	//initialize text and text positions
	text_t T(n);

	itype j = 0;

	cout << "filling skippable text with text characters ... " << flush;

	char c;

	while(ifs.get(c)){

		if(char_to_int[uint8_t(c)] == null){

			char_to_int[uint8_t(c)] = sigma;
			A.push_back(uint8_t(c));
			sigma++;

		}

		T.set(j++,char_to_int[uint8_t(c)]);

	}

	cout << "done. " << endl << endl;

	cout << "alphabet size is " << sigma  << endl << endl;

	cout << "initializing and sorting text positions vector ... " << flush;

	TP_t TP(&T,min_high_frequency);

	cout << "done. Number of text positions containing a high-frequency pair: " << TP.size() << endl;

	//next free dictionary symbol = sigma
	X =  sigma;

	cout << "\nSTEP 1. HIGH FREQUENCY PAIRS" << endl << endl;

	cout << "inserting pairs in high-frequency queue ... " << flush;

	hf_q_t HFQ;
	new_high_frequency_queue(HFQ, TP, T, min_high_frequency);

	cout << "done. Number of distinct high-frequency pairs = " << HFQ.size() << endl;

	cout << "Replacing high-frequency pairs ... " << endl;

	int last_perc = -1;
	uint64_t F = 0;//largest freq

	while(HFQ.max() != HFQ.nullpair()){

		auto f = substitution_round<hf_q_t>(HFQ, TP, T);

		if(last_perc == -1){

			F = f;

			last_perc = 0;

		}else{

			int perc = 100-(100*f)/F;

			if(perc > last_perc+4){

				last_perc = perc;
				cout << perc << "%" << endl;

			}

		}

	}

	cout << "done. " << endl;
	cout << "Peak queue size = " << HFQ.peak() << " (" << double(HFQ.peak())/double(n) << "n)" << endl;

	cout << "\nSTEP 2. LOW FREQUENCY PAIRS" << endl << endl;

	cout << "Re-computing TP array ... " << flush;

	//T.compact(); //remove blank positions
	TP.fill_with_text_positions(); //store here all remaining text positions

	cout << "done." << endl;

	cout << "Sorting  TP array ... " << flush;
	TP.cluster(); //cluster text positions by character pairs
	cout << "done." << endl;


	cout << "Counting low-frequency pairs ... " << flush;
	/*
	 * scan sorted array of text positions and count frequencies
	 *
	 * in this phase, all pairs have frequency < min_high_frequency
	 *
	 * after counting, frequencies[f] is the number of pairs with frequency equal to f
	 *
	 */
	//auto frequencies = vector<uint64_t>(min_high_frequency,0);
	uint64_t n_lf_pairs = 0; //number of low-frequency pairs

	uint64_t f = 1;
	for(uint64_t i=1;i<TP.size();++i){

		if(T.pair_starting_at(TP[i]) == T.pair_starting_at(TP[i-1])){

			f++;

		}else{

			f=1;
			n_lf_pairs++;

		}

	}
	cout << "done. Number of distict low-frequency pairs: "<< n_lf_pairs << endl;

	cout << "Filling low-frequency queue ... " << flush;

	lf_q_t LFQ(min_high_frequency-1);

	f = 1;

	using el_t = lf_q_t::el_type;

	for(uint64_t i=1;i<TP.size();++i){

		if(T.pair_starting_at(TP[i]) == T.pair_starting_at(TP[i-1])){

			f++;

		}else{

			if(f>1){

				cpair ab = T.pair_starting_at(TP[i-1]);

				assert(i>=f);
				itype P_ab = i - f;

				itype L_ab = f;

				itype F_ab = f;

				el_t el = {ab,P_ab,L_ab,F_ab};

				LFQ.insert(el);

			}

			f=1;

		}

	}

	cout << "done." << endl;

	cout << "Replacing low-frequency pairs ... " << endl;

	pair<itype,itype> replaced = {0,0};

	last_perc = -1;
	uint64_t tl = T.number_of_non_blank_characters();

	while(LFQ.max() != LFQ.nullpair()){

		auto f = substitution_round<lf_q_t>(LFQ, TP, T);

		int perc = 100-(100*T.number_of_non_blank_characters())/tl;

		if(perc>last_perc+4){

			last_perc = perc;

			cout << perc << "%" << endl;

		}

	}

	cout << "done. " << endl;
	cout << "Peak queue size = " << LFQ.peak() << " (" << double(LFQ.peak())/double(n) << "n)" << endl;

	cout << "Compressing grammar and storing it to file ... " << endl << endl;

	for(itype i=0;i<T.size();++i){

		if(not T.is_blank(i)) T_vec.push_back(T[i]);

	}

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

	if(argc!=3 and argc != 4) help();

	string mode(argv[1]);

	string in(argv[2]);
	string out;

	if(argc == 4){

		//use output name provided by user
		out = string(argv[3]);

	}else{

		out = string(argv[2]);

		if(mode.compare("c")==0){

			//if compress mode, append .rp
			out.append(".rp");

		}else if(mode.compare("d")==0){


			//if decompress mode, extract extension (if any)
			size_t dot = out.find_last_of(".");
			if (dot != std::string::npos){

				string name = out.substr(0, dot);
				string ext  = out.substr(dot, out.size() - dot);

				//if extension = .rp, remove it. Otherwise, add extension .decompressed
				if(ext.compare(".rp") == 0){

					out = name;

				}else{

					out.append(".decompressed");

				}

			}

		}else{

			help();

		}

	}

	if(not ifstream(in).good()) help();


	if(mode.compare("c")==0){

		cout << "Compressing file " << in << endl;
		cout << "Output will be saved to file " << out << endl<<endl;

		compute_repair(in);

		packed_gamma_file3<> out_file(out);
		//compress the grammar with Elias' gamma-encoding and store it to file
		out_file.compress_and_store(A,G,T_vec);

	}else{

		cout << "Decompressing archive " << in << endl;
		cout << "Output will be saved to " << out << endl;

		auto pgf = packed_gamma_file3<>(in, false);

		vector<itype> A;
		vector<pair<itype,itype> > G;
		vector<itype> Tc;

		//read and decompress grammar (the DAG)
		pgf.read_and_decompress(A,G,Tc);

		ofstream ofs(out);

		//expand the grammar to file
		decompress(A,G,Tc,ofs);

		ofs.close();

		cout << "done." << endl;

	}

}


