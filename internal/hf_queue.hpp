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
 * hf_queue.hpp
 *
 *  Created on: Jan 12, 2017
 *      Author: nico
 *
 *  High-frequency pairs queue
 *
 *  This queue is a pair Q = <H,B> of structures, where:
 *
 *  - H: sigma x sigma -> int is a hash table pointing at elements in B
 *  - B is a linked list storing all high-frequency pairs
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

using namespace std;

#ifndef INTERNAL_HF_QUEUE_HPP_
#define INTERNAL_HF_QUEUE_HPP_


/*
 * template on linked list type and integer type
 */
template<typename ll_type = ll_vec32_t>
class hf_queue{

public:

	using itype = typename ll_type::int_type;
	using ctype = typename ll_type::char_type;

	using cpair = pair<ctype,ctype>;
	using hash_t = std::unordered_map<cpair, itype>;

	using triple_t = triple<itype>;
	using el_type = typename ll_type::el_type;

	/*
	 * default constructor. Note that object must be created with the other constructor in order to be
	 * usable (using object built with this constructor causes failed assertions)
	 */
	hf_queue(){

		max_size = 0;
		min_freq = 0;

	}

	/*
	 * build queue with max capacity equal to max_size and minimum allowed
	 * frequency of a pair equal to min_frequency (included). If a pair'sfrequency
	 * becomes strictly smaller than min_frequency, then the pair is removed from the queue
	 */
	hf_queue(itype max_size, itype min_freq) {

		assert(min_freq>1);

		this->min_freq = min_freq;
		this->max_size = max_size;

		H = hash_t(max_size*2);

	}

	void init(itype max_size, itype min_freq) {

		assert(min_freq>1);

		this->min_freq = min_freq;
		this->max_size = max_size;

		H = hash_t(max_size*2);

	}

	itype minimum_frequency(){
		return min_freq;
	}

	/*
	 * return triple <P_ab, L_ab, F_ab> relative to pair ab
	 * complexity: O(1)
	 */
	triple_t operator[](cpair ab){

		assert(max_size>0);
		assert(ab != nullpair);
		assert(contains(ab));

		auto e = B[H[ab]];

		return {e.P_ab, e.L_ab, e.F_ab};

	}

	/*
	 * return pair with minimum frequency
	 * if the minimum is not synchronized with the content of the queue, re-compute it.
	 */
	cpair min(){

		assert(max_size>0);

		return B.min_pair();

	}

	/*
	 * return pair with maximum frequency
	 * if the maximum is not synchronized with the content of the queue, re-compute it.
	 */
	cpair max(){

		assert(max_size>0);

		return B.max_pair();

	}

	void remove(cpair ab){

		assert(contains(ab));
		assert(max_size>0);
		assert(H[ab] != null);

		B.remove(H[ab]);
		H.erase(ab);

		//if more than half of B's entries are empty, compact B.
		if(B.size() < B.capacity()/2) compact_ll();

		assert(not contains(ab));

	}

	bool contains(cpair ab){

		assert(max_size>0);
		assert(H.count(nullpair) == 0);

		return H.count(ab) == 1;

	}

	/*
	 * current number of pairs in the queue
	 */
	itype size(){

		assert(max_size>0);
		return B.size();

	}

	/*
	 * decrease by 1 F_ab.
	 *
	 */
	void decrease(cpair ab){

		assert(contains(ab));
		assert(H[ab] != null);
		assert(max_size>0);

		assert(B[H[ab]].F_ab > 0);

		B[H[ab]].F_ab--;

	}

	void insert(el_type el){

		assert(max_size>0);

		cpair ab = el.ab;

		assert(not contains(ab));
		assert(el.F_ab >= min_freq);

		itype idx = B.insert(el);
		H.insert({ab,idx});

		//there is at least one pair in the queue (ab), so MAX and MIN must be defined
		assert(min() != nullpair);
		assert(max() != nullpair);
		assert(contains(min()));
		assert(contains(max()));
		assert(H[ab]==idx);
		assert(B[H[ab]].P_ab == el.P_ab);
		assert(B[H[ab]].L_ab == el.L_ab);
		assert(B[H[ab]].F_ab == el.F_ab);
		assert(size()<=max_size);
		assert(contains(ab));

	}

	/*
	 * el must be already in the queue. update values for el
	 */
	void update(el_type el){

		assert(max_size>0);

		cpair ab = el.ab;

		assert(contains(ab));
		assert(el.F_ab >= min_freq);

		B[H[ab]].P_ab = el.P_ab;
		B[H[ab]].L_ab = el.L_ab;
		B[H[ab]].F_ab = el.F_ab;

		//there is at least one pair in the queue (ab), so MAX and MIN must be defined
		assert(min() != nullpair);
		assert(max() != nullpair);
		assert(contains(min()));
		assert(contains(max()));

	}

private:

	/*
	 * compact memory used by the linked list and re-compute
	 * pair's indexes
	 */
	void compact_ll(){

		assert(max_size>0);

		B.compact();

		for(itype i=0;i<B.size();++i){

			auto ab = B[i].ab;
			H[ab] = i;

		}

	}

	itype max_size;
	itype min_freq;

	ll_type B;
	hash_t H;

	const itype null = ~itype(0);

	const cpair nullpair = {null,null};

};

typedef hf_queue<ll_vec32_t> hf_queue32_t;
typedef hf_queue<ll_vec64_t> hf_queue64_t;


#endif /* INTERNAL_HF_QUEUE_HPP_ */
