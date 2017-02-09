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
 * text_positions_lf.hpp
 *
 *  Created on: Jan 20, 2017
 *      Author: nico
 *
 *  array of text positions. Can be sorted by character pairs
 *
 */

#ifndef INTERNAL_TEXT_POSITIONS_LF_HPP_
#define INTERNAL_TEXT_POSITIONS_LF_HPP_

#include "skippable_text_lf.hpp"
#include <unordered_map>

using namespace std;

/*namespace std {
template <> struct hash<std::pair<uint32_t, uint32_t>> {
    inline size_t operator()(const pair<uint32_t, uint32_t> &v) const {
        std::hash<uint32_t> int_hasher;
        return int_hasher(v.first) ^ int_hasher(v.second);
    }
};

namespace std {
template <> struct hash<std::pair<uint64_t, uint64_t>> {
    inline size_t operator()(const pair<uint64_t, uint64_t> &v) const {
        std::hash<uint64_t> int_hasher;
        return int_hasher(v.first) ^ int_hasher(v.second);
    }
};*/


template<typename text_t = skippable_text_lf32_t, typename itype = uint32_t, typename ctype = uint32_t, typename ll_el_t = ll_el32_t>
class text_positions_lf{

public:

	using el_type = ll_el_t;
	using ipair = pair<itype,itype>;
	using cpair = pair<ctype,ctype>;
	using hash_t = std::unordered_map<cpair, vector<itype> >;

	text_positions_lf(){}

	/*
	 * build new array of text positions with only text positions of pairs with
	 * frequency at least min_freq
	 *
	 * assumption: input text is ASCII (max char = 255)
	 *
	 * if max_alphabet_size>0, build table of max_alphabet_size x max_alphabet_size entries to speed-up
	 * pair sorting.
	 *
	 */
	void init(text_t * T, itype max_n_pairs){

		this->T = T;
		this->max_n_pairs = max_n_pairs;

		assert(T->size()>1);

		const itype null = ~itype(0);

		//init Text positions
		TP = vector<itype>(T->size()-1);

		for(itype i=0;i<TP.size();++i) TP[i] = i;

		H = hash_t(max_n_pairs*2);

	}

	/*
	 * get i-th text position
	 */
	itype operator[](itype i){

		assert(i<size());

		return TP[i];

	}

	/*
	 * cluster TP[i,...,j-1] by character pairs. Uses in-place counting-sort
	 */
	void sort(itype i, itype j){

		assert(i<size());
		assert(j<=size());
		assert(i<j);

		//mark in a bitvector only one position per distinct pair
		auto distinct_pair_positions = vector<bool>(j-i,false);

		//first step: count frequencies
		for(itype k = i; k<j; ++k){

			if(not T->is_blank(TP[k])){

				cpair ab = T->pair_starting_at(TP[k]);
				ctype a = ab.first;
				ctype b = ab.second;

				if(ab != nullpair ){

					assert(a<H.size());
					assert(b<H.size());

					//write a '1' iff this is the first time we see this pair
					distinct_pair_positions[k-i] = (H[a][b].first==0);

					H[a][b].first++;

				}

			}

		}

		itype t = i;//cumulated freq

		//second step: cumulate frequencies
		for(itype k = i; k<j; ++k){

			if(distinct_pair_positions[k-i]){

				cpair ab = T->pair_starting_at(TP[k]);
				ctype a = ab.first;
				ctype b = ab.second;

				assert(ab != nullpair);

				assert(a<H.size());
				assert(b<H.size());

				itype temp = H[a][b].first;

				H[a][b].first = t;
				H[a][b].second = t;

				t += temp;

			}

		}

		//itype dist_pairs = 0;

		//clear bitvector content
		for(itype k = i; k<j;++k) distinct_pair_positions[k-i] = false;

		//t is the starting position of null pairs

		itype null_start = t;

		//third step: cluster
		itype k = i; //current position in TP

		//invariant: TP[i,...,k] is clustered
		while(k<j){

			cpair ab = T->pair_starting_at(TP[k]);
			ctype a = ab.first;
			ctype b = ab.second;

			itype ab_start;
			itype ab_end;

			if(ab==nullpair){

				ab_start = null_start;
				ab_end = t;

			}else{

				ab_start = H[a][b].first;
				ab_end = H[a][b].second;

			}

			if(k >= ab_start and k <= ab_end){

				//if k is the first position where a distinct pair (other than nullpair)
				//is seen in the sorted vector, mark it on distinct_pair_positions
				distinct_pair_positions[k-i] = (k==ab_start and ab!=nullpair);

				//dist_pairs += distinct_pair_positions[k-i];

				//case 1: ab is the right place: increment k
				k++;

				if(ab==nullpair){

					t += (ab_end == k);

				}else{

					//if k is exactly next ab position, increment next ab position
					H[a][b].second += (ab_end == k);

				}

			}else{

				//ab has to go to ab_end. swap TP[k] and TP[ab_end]
				itype temp = TP[k];
				TP[k] = TP[ab_end];
				TP[ab_end] = temp;

				if(ab==nullpair){

					t++;

				}else{

					//move forward ab_end since we inserted an ab on top of the list of ab's
					H[a][b].second++;

				}

			}

		}

		//restore H
		for(itype k = i; k<j; ++k){

			if(distinct_pair_positions[k-i]){

				cpair ab = T->pair_starting_at(TP[k]);
				ctype a = ab.first;
				ctype b = ab.second;

				assert(ab!=nullpair);

				H[a][b] = {0,0};

			}

		}

	}

	/*
	 * sort all array
	 */
	void sort(){

		sort(0,size());

	}

	itype size(){

		return TP.size();

	}


private:

	struct comparator {

		comparator(skippable_text_hf<itype,ctype> * T){

			this->T = T;

		}

		bool operator () (int i, int j){

			return T->pair_starting_at(i) < T->pair_starting_at(j);

		}

		text_t * T;

	};

	//a reference to the text
	text_t * T = 0;

	uint64_t width = 0;

	//hash to speed-up pair sorting (to linear time)
	std::unordered_map<cpair,ipair> H; //H[a][b] = <begin, end>. end = next position where to store ab

	//the array of text positions
	//int_vector<> TP;
	vector<itype> TP;

	const itype null = ~itype(0);
	const cpair nullpair = {null,null};

	uint64_t max_n_pairs = 0;//max number of pairs that will be inserted in the queue

};

typedef text_positions_lf<skippable_text_lf32_t, uint32_t, uint32_t, ll_el32_t> text_positions_lf32_t;
typedef text_positions_lf<skippable_text_lf64_t, uint64_t, uint64_t, ll_el64_t> text_positions_lf64_t;

#endif /* INTERNAL_TEXT_POSITIONS_LF_HPP_ */
