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
 * text_positions.hpp
 *
 *  Created on: Jan 20, 2017
 *      Author: nico
 *
 *  array of text positions. Can be sorted by character pairs
 *
 */

#ifndef INTERNAL_TEXT_POSITIONS_HPP_
#define INTERNAL_TEXT_POSITIONS_HPP_

#include <sdsl/int_vector.hpp>
#include <skippable_text.hpp>

using namespace std;
using namespace sdsl;

template<typename itype = uint32_t, typename ctype = uint32_t, typename ll_el_t = ll_el32_t>
class text_positions{

public:

	using el_type = ll_el_t;
	using ipair = pair<itype,itype>;

	text_positions(skippable_text<itype,ctype> * T){

		content = text_pos;

		this->T = T;

		assert(T->size()>1);

		//TP's size: text size - 1
		n = T->size()-1;

		width = 64 - __builtin_clzll(uint64_t(n));

		TP = int_vector<>(n,0,width);

		for(uint64_t i = 0; i<n; ++i) TP[i] = i;

	}

	/*
	 * restore text positions without sorting them
	 */
	void restore_text_positions(){

		assert(content == pairs);

		content = text_pos;

		assert(T->size()>1);

		for(uint64_t i = 0; i<n; ++i) TP[i] = i;

	}

	/*
	 * get i-th text position
	 */
	itype operator[](itype i){

		assert(i<n);
		assert(content == text_pos);

		return TP[i];

	}

	/*
	 * sort TP[i,...,j-1] by character pairs
	 */
	void sort(itype i, itype j){

		assert(content == text_pos);

		assert(i<n);
		assert(j<=n);
		assert(i<j);

		std::sort(TP.begin()+i, TP.begin()+j, comparator(T));

	}

	/*
	 * sort all array
	 */
	void sort(){
		sort(0,size());
	}

	itype size(){

		assert(content == text_pos);
		return n;

	}

	itype number_of_pairs(){

		assert(content == pairs);
		return n_pairs;

	}

	/*
	 * this function turns the object from array of text positions to array of pairs <freq, text_pos>
	 * keep only pairs with minimum frequency min_freq (included)
	 *
	 */
	void extract_frequencies(itype min_freq = 2){

		assert(content == text_pos);

		cout << " sorting ... " << flush;
		std::sort(TP.begin(),TP.end(), comparator(T));
		cout << "done. counting ... " << endl;

		content = pairs;

		itype i = 0; //current write position on TP
		itype j = 0; //current read position on TP
		n_pairs = 0; //number of pairs <freq, text_pos>

		int old_perc = 0;
		int perc;

		while(j<n){

			perc = (100*j)/n;

			if(perc > old_perc+4){
				cout << " " << perc << "%" << endl;
				old_perc=perc;
			}

			itype k = 1; //current pair frequency

			while(	j<TP.size()-1 &&
					T->pair_starting_at(TP[j]) != T->blank_pair() &&
					T->pair_starting_at(TP[j]) == T->pair_starting_at(TP[j+1]) ){

				j++;
				k++;

			}

			if(k>=min_freq){

				itype text_pos = TP[j];
				TP[i] = k;
				TP[i+1] = text_pos;
				i += 2;
				n_pairs++;

				//cout << T->pair_starting_at(TP[j]).first << ", " << T->pair_starting_at(TP[j]).second << " -> " << k << endl;

			}

			//advance to next pair (or to end of TP)
			j++;

		}

		cout << " done." << endl;

	}

	/*
	 * get i-th pair <freq, text_pos>.
	 * this object must be in the state 'frequencies' in order to call this function
	 */
	ipair get_pair(itype i){

		assert(content == pairs);
		assert(i<n_pairs);

		return {TP[2*i],TP[2*i+1]};

	}


	/*
	 * return frequencies of all pairs in a compact int_vector. Frequencies are sorted in decreasing order
	 */
	int_vector<> get_sorted_pair_frequencies(){

		assert(content == pairs);

		int_vector<> result = int_vector<>(n_pairs,0,width);

		for(uint64_t i=0;i<n_pairs;++i){

			result[i] = get_pair(i).first;

		}

		std::sort(result.begin(),result.end(), [](uint64_t x, uint64_t y) -> bool {return x>y;} );

		return result;

	}


private:

	struct comparator {

		comparator(skippable_text<itype,ctype> * T){

			this->T = T;

		}

		bool operator () (int i, int j){

			return T->pair_starting_at(i) < T->pair_starting_at(j);

		}

		skippable_text<itype,ctype> * T;

	};

	/*
	 * this object can exist in 2 formats: text positions and pairs <freq, text_pos>, where freq is the frequency of the pair
	 * starting at text position text_pos
	 *
	 *
	 *
	 */
	//const bool text_pos = 0;
	//const bool pairs = 1;

	enum content_t {text_pos, pairs};
	content_t content;

	//int content;

	//if content = frequencies, this is the number of pairs stored in the structure
	//and equals the number of distinct text pairs with frequency at least 2
	itype n_pairs = 0;

	//a reference to the text
	skippable_text<itype,ctype> * T;

	//TP's size: text size - 1
	itype n;

	uint64_t width;

	//the array of text positions
	int_vector<> TP;

};

typedef text_positions<uint32_t, uint32_t, ll_el32_t> text_positions32_t;
typedef text_positions<uint64_t, uint64_t, ll_el64_t> text_positions64_t;

#endif /* INTERNAL_TEXT_POSITIONS_HPP_ */
