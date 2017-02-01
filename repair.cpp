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

#include <skippable_text.hpp>
#include <text_positions.hpp>

#include <fstream>
#include <math.h>

#include <sdsl/int_vector.hpp>

#include "internal/hf_queue.hpp"

using namespace std;
using namespace sdsl;

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
void synchronize_hf(queue_t & Q, TP_t & TP, text_t & T, cpair AB){

	assert(not Q.contains(Q.max()) || Q[Q.max()].F_ab >= Q.minimum_frequency());

	assert(Q.size() > 0);
	assert(Q[Q.max()].F_ab >= Q.minimum_frequency());

	//variables associated with AB
	assert(Q.contains(AB));
	auto q_el = Q[AB];
	itype P_AB = q_el.P_ab;
	itype L_AB = q_el.L_ab;
	itype F_AB = q_el.F_ab;

	itype freq_AB = 0;//number of pairs AB seen inside the interval. Computed inside this function

	assert(P_AB+L_AB <= TP.size());
	//sort sub-array corresponding to AB
	TP.sort(P_AB,P_AB+L_AB);

	//scan TP[P_AB,...,P_AB+L_AB-1] and detect new pairs
	itype j = P_AB;//current position in TP
	while(j<P_AB+L_AB){

		itype p = j; //starting position of current pair in TP
		itype k = 1; //current pair frequency

		cpair XY = T.blank_pair();

		while(	j<(P_AB+L_AB)-1 &&
				T.pair_starting_at(TP[j]) != T.blank_pair() &&
				T.pair_starting_at(TP[j]) == T.pair_starting_at(TP[j+1]) ){

			XY = T.pair_starting_at(TP[j]);
			j++;
			k++;

		}

		//if the pair is not AB and it is a high-frequency pair, insert it in queue
		if(XY != AB and k >= Q.minimum_frequency()){

			assert(XY != T.blank_pair());
			assert(not Q.contains(XY));

			Q.insert({XY,p,k,k}); //minimum is automatically updated here if k is smaller than the new minimum

		}else if(XY == AB){ //the pair is AB and is already in the queue: update its frequency

			freq_AB = k;

			assert(Q.contains(AB));

			//update only if frequency is at least minimum frequency (otherwise we will remove the pair afterwards)
			if(k >= Q.minimum_frequency()) Q.update({AB,p,k,k});

		}

		j++;

	}

	assert(Q.contains(AB));

	//it could be that now AB's frequency is too small: delete it
	if(freq_AB < Q.minimum_frequency()){

		Q.remove(AB);//automatically re-computes min/max

	}

	//Q does not contain its max -> Q.size() == 0
	assert(Q.contains(Q.max()) || Q.size() == 0);
	assert(not Q.contains(Q.max()) || Q[Q.max()].F_ab >= Q.minimum_frequency());
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

		synchronize_hf<queue_t>(Q, TP, T, ab);

	}else{

		if(F_ab < Q.minimum_frequency()){

			Q.remove(ab);

		}

	}

}

template<typename queue_t>
void substitution_round(queue_t & Q, TP_t & TP, text_t & T){

	using ctype = text_t::char_type;

	//compute max and min
	cpair AB = Q.max();

	assert(Q.contains(AB));

	assert(Q[AB].F_ab >= Q.minimum_frequency());

	//extract P_AB and L_AB
	auto q_el = Q[AB];
	itype F_AB = q_el.F_ab;
	itype P_AB = q_el.P_ab;
	itype L_AB = q_el.L_ab;

	cout << " extracted MAX = " << AB.first << " " << AB.second << " (frequency = " << F_AB << ")" << endl;

	//output new rule
	cout << " new rule: " << X << " -> " << AB.first << " " << AB.second << endl;

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

				assert(Q[xA].F_ab > 0);

				Q.decrease(xA);

			}

			if(Q.contains(By) && By != AB){

				assert(Q[By].F_ab > 0);

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
	synchronize_hf<queue_t>(Q, TP, T, AB); //automatically removes AB since new AB's frequency is 0
	assert(not Q.contains(AB));

	assert(Q.contains(Q.max()) or Q.size() == 0);
	assert(not Q.contains(Q.max()) || Q[Q.max()].F_ab >= Q.minimum_frequency());

	//advance next free dictionary symbol
	X++;

	cout << " current text size = " << T.number_of_non_blank_characters() << endl << endl;

}

void compute_repair(string in, string out_rp, string out_g){

	/*
	 * tradeoff between low-frequency and high-freq phase:
	 *
	 * - High-freq phase will use n^(2 - 2*alpha) words of memory and process approximately n^(1-alpha) pairs
	 *   alpha should satisfy 0.5 < alpha < 1
	 *
	 * - Low-freq phase will use n^alpha words of memory
	 *
	 */
	double alpha = 0.66;

	itype n;
	itype sigma = 0; //alphabet size

	/*
	 * square root of n. Pairs with frequency greater than or equal to sqrt_n are inserted in high-freq queue
	 */
	itype min_high_frequency = 0;
	itype lf_queue_capacity = 0;

	/*
	 * (1) INITIALIZE DATA STRUCTURES
	 */

	const itype null = ~itype(0);

	ifstream ifs(in);

	vector<itype> char_to_int(256,null);
	vector<itype> int_to_char(256,null);

	//count file size
	{
		ifstream file(in,ios::ate);
		n = file.tellg();
	}

	//min_high_frequency = std::sqrt(n);

	min_high_frequency = std::pow(  n, alpha  );// n^(alpha)

	min_high_frequency = min_high_frequency <2 ? 2 : min_high_frequency;

	cout << "File size = " << n << " characters"  << endl;
	cout << "cut-off frequency = " << min_high_frequency  << endl;

	itype width = 64 - __builtin_clzll(uint64_t(n));

	cout << "log_2 n = " << width << " bits"  << endl;

	//largest possible high-frequency dictionary symbol
	//at most max_d <= n high-freq dictionary symbols can be created; given that min. freq of
	//a high-freq dictionary symbol is f=min_high_frequency and that every new dictionary symbol
	//introduces a blank in the text, we have the inequality max_d * f <= n  <=> max_d <= n/f
	itype max_d = 256+n/min_high_frequency;

	cout << "Max high-frequency dictionary symbol = " << max_d << endl << endl;

	//initialize text and text positions
	text_t T(n,max_d);

	itype j = 0;

	cout << "filling skippable text with text characters ... " << flush;

	char c;
	while(ifs.get(c)){

		if(char_to_int[uint8_t(c)] == null){

			char_to_int[uint8_t(c)] = sigma;
			int_to_char[sigma] = uint8_t(c);

			sigma++;

		}

		T.set(j++,char_to_int[uint8_t(c)]);

	}

	cout << "done. " << endl << endl;

	cout << "alphabet size is " << sigma  << endl << endl;

	cout << "initializing and sorting text positions vector ... " << flush;

	TP_t TP(&T,min_high_frequency,max_d);

	cout << "done. Number of text positions containing a high-frequency pair: " << TP.size() << endl;

	//for(uint64_t i = 0; i<n; ++i) cout << (uint8_t)int_to_char[T[i]];cout<<endl;

	//next free dictionary symbol = sigma
	X =  sigma;

	cout << "STEP 1. HIGH FREQUENCY PAIRS" << endl << endl;

	//cout << "sorting text positions by character pairs ... " << flush;

	//TP.sort();

	//cout << "done." << endl;

	cout << "inserting pairs in high-frequency queue ... " << flush;

	hf_q_t HFQ;
	new_high_frequency_queue(HFQ, TP, T, min_high_frequency);

	cout << "done. Number of distinct high-frequency pairs = " << HFQ.size() << endl;


	cout << "Replacing high-frequency pairs ... " << endl;

	while(HFQ.size() > 0){

		substitution_round<hf_q_t>(HFQ, TP, T);

	}

	cout << "done. " << endl;


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
	cout << "Output will be saved to files " << out_rp  << " and " << out_g << endl<<endl;

	compute_repair(in, out_rp, out_g);

}


