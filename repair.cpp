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
#include <set>

#include <hf_queue.hpp>
#include <lf_queue.hpp>

#include <ll_vec.hpp>
#include <ll_el.hpp>

#include <skippable_text.hpp>
#include <text_positions.hpp>

#include <fstream>
#include <math.h>

#include <sdsl/int_vector.hpp>

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
using hf_q_t = hf_queue32_t; //we insert ALL high-freq pairs in the high-freq queue
using lf_q_t = lf_queue32_t;
using cpair = hf_q_t::cpair;
using itype = uint32_t;
//using ctype = uint32_t;

//next free dictionary symbol
itype X=0;

/*
 * Given (empty) queue, text positions, text, and minimum frequency: insert in Q all pairs with frequency at least min_freq.
 *
 * assumptions: TP is sorted by character pairs, Q is void
 *
 */
void new_high_frequency_queue(hf_q_t & Q, TP_t & TP, text_t & T, uint64_t min_freq = 2){

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

	//create new queue. Allocate space for n_pairs*2 pairs in the hash to guarantee a
	//load factor <= 0.5
	Q.init(n_pairs*2,min_freq);

	j = 0;

	/*
	 * step 2. Fill queue
	 */
	while(j<n){

		itype P_ab = j; //starting position in TP of pair

		perc = (100*j)/n;

		if(perc > old_perc+4){
			cout << " " << perc << "%" << endl;
			old_perc=perc;
		}

		itype k = 1; //current pair frequency
		cpair ab;

		while(	j<TP.size()-1 &&
				T.pair_starting_at(TP[j]) != T.blank_pair() &&
				T.pair_starting_at(TP[j]) == T.pair_starting_at(TP[j+1]) ){

			ab = T.pair_starting_at(TP[j]);

			j++;
			k++;

		}

		if(k>=min_freq){

			//cout << "\ninserting " << ab.first << " " << ab.second << ", " << P_ab << ", " << k << ", " << k << endl;

			Q.insert({ab, P_ab, k, k});

		}

		j++;

	}

}

/*
 * synchronize queue in range corresponding to pair AB. Provide also reference to minimum pair ab (procedure may modify ab)
 */
template<typename queue_t>
void synchronize_hf(queue_t & Q, TP_t & TP, text_t & T, cpair & ab, cpair AB, bool AB_forbidden = false){

	auto q_el = Q[ab];
	//frequency of minimum element
	itype F_ab = q_el.F_ab;

	//coordinates of AB

	assert(Q.contains(AB));
	q_el = Q[AB];
	itype P_AB = q_el.P_ab;
	itype L_AB = q_el.L_ab;
	itype F_AB = q_el.F_ab;

	cout << "synchronizing pair = " << AB.first << " " << AB.second << " with L, F = " << L_AB << " " << F_AB << endl;

	//sort sub-array corresponding to AB
	TP.sort(P_AB,P_AB+L_AB);

	/*
	 * scan TP[P_AB,...,P_AB+L_AB-1] and detect new pairs
	 */

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

		if(XY!=T.blank_pair())
		cout << "Detected non-blank pair : " << XY.first << " " << XY.second << ". freq = " << k << endl;

		if(XY != AB and k > F_ab){

			assert(XY != T.blank_pair());

			//XY has a higher frequency than minimum pair: remove minimum pair and insert XY
			Q.remove(ab);

			/*cout << "1. inserting pair with freq = " << k << endl;
			cout << "pair = " << XY.first << " " << XY.second << endl;
			cout << "coord = " << p << " " << k << endl;
*/
			assert(not Q.contains(XY));

			Q.insert({XY,p,k,k});

			//re-compute minimum and its frequency
			ab = Q.min();
			assert(Q.contains(ab));
			F_ab = Q[ab].F_ab;

		}else if(XY == AB){

			assert(not AB_forbidden);

			//remove and re-insert AB with its new values for P, F, and L
			Q.remove(AB);
/*
			cout << "2. inserting pair with freq = " << k << endl;
			cout << "L_AB was " << L_AB << endl;*/
			Q.insert({AB,p,k,k});

			if(k<F_ab){

				ab = AB;
				F_ab = k;

			}

		}

		j++;

	}

}


template<typename queue_t>
void substitution_round(queue_t & Q, TP_t & TP, text_t & T){

	using ctype = text_t::char_type;

	//compute max and min
	cpair AB = Q.max();
	cpair ab = Q.min();

	//extract P_AB and L_AB
	auto q_el = Q[AB];
	itype F_AB = q_el.F_ab;
	itype P_AB = q_el.P_ab;
	itype L_AB = q_el.L_ab;

	q_el = Q[ab];
	itype F_ab = q_el.F_ab;

	cout << "\nMIN = " << ab.first << " " << ab.second << " (" << F_ab << ")" << endl;
	cout << "MAX = " << AB.first << " " << AB.second << " (" << F_AB << ")" << endl;

	//outpout new rule
	cout << "new rule: " << X << " -> " << AB.first << " " << AB.second << endl;

	for(itype j = P_AB; j<P_AB+L_AB;++j){

		itype i = TP[j];

		if(T.pair_starting_at(i) == AB){

			//the context of AB is xABy. We now extract AB's context:
			cpair xA = T.pair_ending_at(i);
			cpair By = T.next_pair(i);

			assert(By == T.blank_pair() or By.first == AB.second);

			//note: xA and By could be blank pairs if this AB was the first/last pair in the text

			//perform replacement
			T.replace(i,X);

			if(xA != T.blank_pair() && Q.contains(xA) && xA != AB){

				Q.decrease(xA);

			}

			if(By != T.blank_pair() && Q.contains(By) && xA != AB){

				Q.decrease(By);

			}

		}

	}

	/*
	 * re-scan text positions associated to AB and synchronize if needed
	 */
	for(itype j = P_AB; j<P_AB+L_AB;++j){

		itype i = TP[j];

		assert(T.pair_starting_at(i) != AB); //we replaced all AB's ...

		if(T[i] == X){

			//the context of X is xX. We now extract X's left context:
			cpair xX = T.pair_ending_at(i);

			//careful: x could be = X. in this case, before the replacements this xX was equal to ABAB -> a BA disappeared
			ctype x = xX.first == X ? AB.second : xX.first;
			ctype A = AB.first;

			//this is the pair that disappeared
			cpair xA = xX == T.blank_pair() ? xX : cpair {x,A};


			if(xA != T.blank_pair() && Q.contains(xA) && xA != AB){

				q_el = Q[xA];
				itype F_xA = q_el.F_ab;
				itype L_xA = q_el.L_ab;

				if(F_xA <= L_xA/2){

					cout << "synchronize other pair " << endl;

					assert(xA != AB);

					synchronize_hf<queue_t>(Q, TP, T, ab, xA);

					cout << "new values: " << F_xA << " / " <<  L_xA << endl;
					assert(Q[xA].F_ab == Q[xA].L_ab);

				}

			}

		}

	}

	cout << "synchronize replaced pair " << endl;
	synchronize_hf<queue_t>(Q, TP, T, ab, AB, true);

	Q.remove(AB);

	//advance next free dictionary symbol
	X++;

}

void compute_repair(string in, string out_rp, string out_g){

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

	min_high_frequency = std::sqrt(n);
	min_high_frequency = min_high_frequency <2 ? 2 : min_high_frequency;

	cout << "File size = " << n << " characters"  << endl;
	cout << "cut-off frequency = " << min_high_frequency  << endl;

	itype width = 64 - __builtin_clzll(uint64_t(n));

	cout << "log_2 n = " << width << " bits"  << endl;

	cout << endl << "initializing text position vector and skippable text ... " << flush;

	//initialize text and text positions
	text_t T(n);
	TP_t TP(&T);

	cout << "done." << endl;

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

	cout << "done. " << endl;

	//for(uint64_t i = 0; i<n; ++i) cout << (uint8_t)int_to_char[T[i]];cout<<endl;

	//next free dictionary symbol = sigma
	X =  sigma;

	cout << "alphabet size is " << sigma  << endl << endl;

	cout << "STEP 1. HIGH FREQUENCY PAIRS" << endl << endl;

	cout << "sorting text positions by character pairs ... " << flush;

	TP.sort();

	cout << "done." << endl;

	cout << "inserting pairs in high-frequency queue ... " << flush;

	hf_q_t HFQ;
	new_high_frequency_queue(HFQ, TP, T, min_high_frequency);

	cout << "done. Number of high-frequency pairs = " << HFQ.size() << endl;


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


