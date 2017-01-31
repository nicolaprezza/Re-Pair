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
 * hf_queue_v2_v2.hpp
 *
 *  Created on: Jan 12, 2017
 *      Author: nico
 *
 *  High-frequency pairs queue implemented by direct-addressing pairs
 *
 *  Supported operations:
 *
 *  operator[ab]: return triple <P_ab, L_ab, F_ab> relative to pair ab
 *  max()/min(): return pair ab with max/min F_ab
 *  remove(ab): delete pair ab from queue
 *  contains(ab): true iff ab is in the queue
 *  size(): current queue size
 *  decrease(ab): decrease by 1 ab's frequency F_ab. This function removes ab if its frequency goes below the queue's min frequency
 *  insert(list_el), where list_el = <ab, P_ab, L_ab, F_ab> is a linked list element
 *
 *
 */

#include <ll_vec.hpp>
#include <unordered_map>
#include <ll_el.hpp>
#include <pair_hash.hpp>

using namespace std;

#ifndef INTERNAL_HF_QUEUE_V2_HPP_
#define INTERNAL_HF_QUEUE_V2_HPP_

/*
 * template on linked list type and integer type
 */
template<typename el_type = ll_el32_t, typename itype = uint32_t, typename ctype = uint32_t>
class hf_queue_v2{

public:

	using triple_t = triple<itype>;

	typedef pair_hash<triple_t,itype,ctype> hash_t;

	using cpair = pair<ctype,ctype>;
	//using hash_t = std::unordered_map<cpair, itype>;

	/*
	 * default constructor. Note that object must be created with the other constructor in order to be
	 * usable (using object built with this constructor causes failed assertions)
	 */
	hf_queue_v2(){

		min_freq = 0;

	}

	hf_queue_v2(itype max_alphabet_size, itype min_freq) {

		assert(min_freq>1);

		this->min_freq = min_freq;

		H = hash_t(max_alphabet_size, triple_t());

	}

	void init(itype max_alphabet_size, itype min_freq) {

		assert(min_freq>1);

		this->min_freq = min_freq;

		H.init(max_alphabet_size, triple_t());

	}

	itype minimum_frequency(){
		return min_freq;
	}

	/*
	 * return triple <P_ab, L_ab, F_ab> relative to pair ab
	 * complexity: O(1)
	 */
	triple_t operator[](cpair ab){

		assert(ab != nullpair);
		assert(contains(ab));

		return H[ab];

	}

	/*
	 * return pair with minimum frequency
	 */
	cpair min(){

		if(n==0) return nullpair;

		itype min_F = null;
		cpair min_pair = nullpair;

		for(cpair p : pairs_in_hash){

			if(contains(p)){

				auto f = H[p].F_ab;

				if(f < min_F){

					min_F = f;
					min_pair = p;

				}

			}

		}

		assert(min_pair != nullpair);
		return min_pair;

	}

	/*
	 * return pair with maximum frequency
	 */
	cpair max(){

		if(n==0) return nullpair;

		itype max_F = 0;
		cpair max_pair = nullpair;

		for(cpair p : pairs_in_hash){

			if(contains(p)){

				auto f = H[p].F_ab;

				if(f > max_F){

					max_F = f;
					max_pair = p;

				}

			}

		}

		assert(max_pair != nullpair);
		return max_pair;

	}

	void remove(cpair ab){

		assert(contains(ab));
		assert(H[ab] != H.null_el());

		H.erase(ab);

		assert(not contains(ab));

		n--;

	}

	bool contains(cpair ab){

		assert(H.count(nullpair) == 0);

		return H.count(ab) == 1;

	}

	/*
	 * current number of pairs in the queue
	 */
	itype size(){

		return n;

	}

	/*
	 * decrease by 1 F_ab. Does not remove pair!
	 *
	 */
	void decrease(cpair ab){

		assert(contains(ab));
		assert(H[ab] != H.null_el());

		assert(H[ab].F_ab > 0);

		H[ab].F_ab--;

	}

	void insert(el_type el){

		cpair ab = el.ab;

		assert(not contains(ab));
		assert(el.F_ab >= min_freq);

		H.insert({ab,triple_t {el.P_ab, el.L_ab, el.F_ab}});

		pairs_in_hash.push_back(ab);

		n++;

		//there is at least one pair in the queue (ab), so MAX and MIN must be defined
		assert(min() != nullpair);
		assert(max() != nullpair);
		assert(contains(min()));
		assert(contains(max()));
		assert(H[ab].P_ab == el.P_ab);
		assert(H[ab].L_ab == el.L_ab);
		assert(H[ab].F_ab == el.F_ab);
		assert(contains(ab));

	}

	/*
	 * el must be already in the queue. update values for el
	 */
	void update(el_type el){

		cpair ab = el.ab;

		assert(contains(ab));
		assert(el.F_ab >= min_freq);

		H[ab].P_ab = el.P_ab;
		H[ab].L_ab = el.L_ab;
		H[ab].F_ab = el.F_ab;

		//there is at least one pair in the queue (ab), so MAX and MIN must be defined
		assert(min() != nullpair);
		assert(max() != nullpair);
		assert(contains(min()));
		assert(contains(max()));

	}

private:

	itype min_freq;

	hash_t H;
	vector<cpair> pairs_in_hash;

	const itype null = ~itype(0);

	const cpair nullpair = {null,null};

	itype n = 0;//number of pairs in queue

};

typedef hf_queue_v2<ll_el32_t, uint32_t, uint32_t> hf_queue_v2_32_t;
typedef hf_queue_v2<ll_el64_t, uint32_t, uint32_t> hf_queue_v2_64_t;


#endif /* INTERNAL_HF_QUEUE_V2_HPP_ */
