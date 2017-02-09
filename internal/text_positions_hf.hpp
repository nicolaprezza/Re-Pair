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
 * text_positions_hf.hpp
 *
 *  Created on: Jan 20, 2017
 *      Author: nico
 *
 *  array of text positions. Can be sorted by character pairs
 *
 */

#ifndef INTERNAL_TEXT_POSITIONS_HF_HPP_
#define INTERNAL_TEXT_POSITIONS_HF_HPP_

#include "skippable_text_hf.hpp"
#include <algorithm>

using namespace std;

template<typename itype = uint32_t, typename ctype = uint32_t, typename ll_el_t = ll_el32_t>
class text_positions_hf{

public:

	using el_type = ll_el_t;
	using ipair = pair<itype,itype>;
	using cpair = pair<ctype,ctype>;
	using hash_t = std::unordered_map<cpair, vector<itype> >;

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
	text_positions_hf(skippable_text_hf<itype,ctype> * T, itype min_freq){

		//hash will be of size maxd*maxd words
		uint64_t maxd = std::max(uint64_t(std::pow(  T->size(), 0.4  )),uint64_t(256));

		//hash to accelerate sorting of pairs
		H = vector<vector<ipair> >(maxd,vector<ipair>(maxd,{0,0}));

		this->T = T;

		assert(T->size()>1);

		//frequency of every possible ASCII pair
		auto F = vector<vector<itype> >(256,vector<itype>(256,0));

		//count frequencies
		for(itype i = 0;i<T->size()-1;++i){

			cpair p = T->pair_starting_at(i);
			ctype a = p.first;
			ctype b = p.second;

			assert(p != T->blank_pair());

			assert(a<256);
			assert(b<256);

			F[a][b]++;

		}

		const itype null = ~itype(0);

		itype hf_pairs = 0;

		for(ctype a = 0;a<256;++a){

			for(ctype b = 0;b<256;++b){

				itype t = F[a][b];

				if(F[a][b] < min_freq){

					F[a][b] = null;

				}else{

					F[a][b] = hf_pairs;
					hf_pairs += t;

				}

			}

		}

		//TP = int_vector<>(hf_pairs,0,width);
		TP = vector<itype>(hf_pairs,0);

		//fill TP: cluster high-freq pairs
		for(itype i = 0;i<T->size()-1;++i){

			cpair p = T->pair_starting_at(i);
			ctype a = p.first;
			ctype b = p.second;

			assert(a<256);
			assert(b<256);

			if(F[a][b] != null){//if ab is a high-freq pair

				assert(F[a][b] < TP.size());

				//store i at position F[a][b], increment F[a][b]
				TP[ F[a][b]++ ] = i;

			}

		}

	}

	/*
	 * resize TP's size to the text size n
	 *
	 * replace content with text positions 0,...,n-1
	 *
	 * WARNING: this function does not sort
	 *
	 */
	void resize(){

		TP.resize(T->size());
		//TP.shrink_to_fit();

		itype i=0;
		for(auto & x : TP) x = i++;

	}

	/*
	 * get i-th text position
	 */
	itype operator[](itype i){

		assert(i<size());

		return TP[i];

	}

	void nlogn_sort(itype i, itype j){

		std::sort(TP.begin()+i, TP.begin()+j,comparator(T));

	}

	/*
	 * cluster TP[i,...,j-1] by character pairs. Uses in-place counting-sort
	 */
	void sort(itype i, itype j){

		/*
		 * if the largest symbol in the text is too big for the hash,
		 * just apply slow comparison-sort
		 */
		if(T->get_max_symbol() >= H.size()){

			nlogn_sort(i,j);
			return;

		}

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

	void nlogn_sort(){
		nlogn_sort(0,size());
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

		skippable_text_hf<itype,ctype> * T;

	};

	//a reference to the text
	skippable_text_hf<itype,ctype> * T;

	//hash to speed-up pair sorting (to linear time)
	vector<vector<ipair> > H; //H[a][b] = <begin, end>. end = next position where to store ab

	//the array of text positions
	//int_vector<> TP;
	vector<itype> TP;

	const itype null = ~itype(0);
	const cpair nullpair = {null,null};

};

typedef text_positions_hf<uint32_t, uint32_t, ll_el32_t> text_positions_hf32_t;
typedef text_positions_hf<uint64_t, uint64_t, ll_el64_t> text_positions_hf64_t;

#endif /* INTERNAL_TEXT_POSITIONS_HF_HPP_ */
