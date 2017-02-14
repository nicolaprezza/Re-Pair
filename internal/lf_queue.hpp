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
 * lf_queue.hpp
 *
 *  Created on: Jan 16, 2017
 *      Author: nico
 *
 *  Low-frequency pairs queue
 *
 *  This queue is a pair Q = <F,H> of structures, where:
 *
 *  - F: is a doubly-linked frequency vector indexing all possible pair frequencies smaller than
 *    a pre-defoined quantity. Each F's entry (frequency) is associated to a linked list containing all pairs
 *    with that frequency
 *
 *  - H: sigma x sigma -> int is a hash table pointing at elements in B
 *
 *  Supported operations (all amortized constant time)
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

#ifndef INTERNAL_LF_QUEUE_HPP_
#define INTERNAL_LF_QUEUE_HPP_

#include <ll_vec.hpp>
#include <unordered_map>
#include <ll_el.hpp>

using namespace std;


/*
 * template on linked list type and integer type
 */
template<typename ll_type = ll_vec32_t>
class lf_queue{

using itype = typename ll_type::int_type;
using ctype = typename ll_type::char_type;

using cpair = pair<ctype,ctype>;
using ipair = pair<itype,itype>;

//value of hash elements: pair <frequency, offset>. The element is accessed as F[frequency].list[offset]
using hash_t = std::unordered_map<cpair, ipair>;

public:

	using int_type = itype;
	using char_type = ctype;

	using triple_t = triple<itype>;
	using el_type = typename ll_type::el_type;

	/*
	 * default constructor. Note that object must be created with the other constructor in order to be
	 * usable (using object built with this constructor causes failed assertions)
	 */
	lf_queue(){

		max_size = 0;
		max_freq = 0;

	}

	lf_queue(itype max_size, itype max_freq) {

		assert(max_freq>0);

		this->max_freq = max_freq;
		//this->max_size = max_size;
		this->max_size = ~(itype(0)); //for now, unlimited max size

		F = vector<ll_type>(max_freq+1);

		MAX = max_freq;

		H = hash_t(max_size*2);
		H.max_load_factor(0.6);

	}

	/*
	 * return triple <P_ab, L_ab, F_ab> relative to pair ab
	 * complexity: O(1)
	 */
	triple_t operator[](cpair ab){

		assert(check_hash_consistency());

		assert(max_size>0);
		assert(H[ab].second < F[H[ab].first].capacity());

		assert(contains(ab));

		assert(check_hash_consistency());

		auto coord = H[ab];

		auto freq = coord.first;
		auto offset = coord.second;

		assert(freq < F.size());
		assert(offset < F[freq].capacity());
		assert(not F[freq][offset].is_null());

		auto e = F[freq][offset];

		assert(e.F_ab == freq);
		assert(e.F_ab <= max_freq);

		return {e.P_ab, e.L_ab, e.F_ab};

	}

	/*
	 * return pair with highest frequency. If queue is empty, return null pair
	 */
	cpair max(){

		if(n==0) return nullpair;

		assert(check_hash_consistency());

		assert(max_size>0);
		assert(MAX<F.size());
		assert(MAX>1);

		while(MAX > 1 && F[MAX].size() == 0){MAX--;}

		assert(MAX>1);

		auto ab = F[MAX].head();

		assert(H[ab].second < F[H[ab].first].capacity());
		assert(contains(ab));
		assert(F[H[ab].first][H[ab].second].ab == ab);
		assert(H[ab].first == MAX);
		assert(F[H[ab].first][H[ab].second].F_ab == MAX);
		assert(not F[H[ab].first][H[ab].second].is_null());
		assert(check_hash_consistency());

		return ab;

	}

	void remove(cpair ab){

		assert(check_hash_consistency());

		assert(contains(ab));
		assert(max_size>0);
		assert(H[ab].second < F[H[ab].first].capacity());

		auto coord = H[ab];
		auto freq = coord.first;
		auto offset = coord.second;

		assert(freq < F.size());
		assert(freq <= max_freq);
		assert(offset < F[freq].capacity());
		assert(not F[freq][offset].is_null());
		assert(F[freq][offset].ab == ab);

		F[freq].remove(offset);//remove pair from linked list
		H.erase(ab);//remove pair from hash

		assert(check_hash_consistency());

		//if more than half of B's entries are empty, compact B.
		if(F[freq].size() < F[freq].capacity()/2)
			compact_ll(freq);

		assert(check_hash_consistency());

		assert(n>0);

		//decrease size
		n--;

		assert(check_hash_consistency());

	}

	bool contains(cpair ab){

		//assert(check_hash_consistency());

		assert(max_size>0);
		//assert(H.count(ab) == 0 or H[ab].second < F[H[ab].first].list.capacity());

		return H.count(ab) == 1;

	}

	itype size(){

		assert(check_hash_consistency());

		assert(max_size>0);

		return n;

	}

	/*
	 * decrease by 1 F_ab
	 */
	void decrease(cpair ab){

		assert(check_hash_consistency());

		assert(contains(ab));
		assert(max_size>0);
		assert(H[ab].second < F[H[ab].first].capacity());

		auto coord = H[ab];

		auto freq = coord.first;
		auto offset = coord.second;

		assert(freq > 1);
		assert(freq < F.size());
		assert(offset < F[freq].capacity());
		assert(not F[freq][offset].is_null());

		//copy element
		el_type e = el_type(F[freq][offset]);

		assert(ab == e.ab);
		assert(e.F_ab == freq);
		assert(e.F_ab >= 2);
		assert(freq <= max_freq);

		//remove pair ab from the queue as its frequency changed
		remove(ab);
		assert(not contains(ab));

		//decrease frequency
		e.F_ab--;

		assert(check_hash_consistency());

		//if ab's frequency becomes equal to 1, do not re-insert ab in the queue
		if(e.F_ab < 2) return;

		//now insert the element in its list
		auto off = F[e.F_ab].insert(e);

		assert(off < F[e.F_ab].capacity());
		assert(not contains(ab));

		//update hash with new coordinates of the pair ab
		H.insert({ab,{e.F_ab,off}});

		assert(contains(ab));

		//increase size counter ( it was decreased inside remove() )
		n++;

		assert(H[ab].second < F[H[ab].first].capacity());

		assert(check_hash_consistency());

	}

	/*
	 * insert an element in the queue.
	 * Warning: this function assumes that element's frequency e.F_ab is
	 * already recorded in the internal linked lists
	 *
	 */
	void insert(el_type el){

		assert(check_hash_consistency());

		itype f = el.F_ab;
		cpair ab = el.ab;

		assert(f>=2);
		assert(f < F.size());
		assert(not contains(ab));
		assert(max_size>0);

		//insert ab in its list
		auto off = F[f].insert(el);
		assert(off < F[f].capacity());

		assert(check_hash_consistency());

		//insert ab in the hash
		H.insert({ab,{f,off}});

		assert(check_hash_consistency());

		n++;

		assert(H[ab].second < F[H[ab].first].capacity());
		assert(n <= max_size);

		assert(check_hash_consistency());

	}

	itype minimum_frequency(){
		return 2;
	}

	/*
	 * el must be already in the queue. update L_ab value for el
	 *
	 * note that el.F_ab must be the same of the frequency stored in the queue
	 *
	 */
	void update(el_type el){

		cpair ab = el.ab;

		assert(contains(ab));

		assert(check_hash_consistency());

		assert(contains(ab));
		assert(max_size>0);
		assert(H[ab].second < F[H[ab].first].capacity());

		auto coord = H[ab];

		auto freq = coord.first;
		auto offset = coord.second;

		assert(freq == el.F_ab);

		assert(freq > 1);
		assert(freq < F.size());
		assert(offset < F[freq].capacity());
		assert(not F[freq][offset].is_null());
		assert(F[freq][offset].L_ab >= el.L_ab);

		//copy element
		F[freq][offset].P_ab = el.P_ab;
		F[freq][offset].L_ab = el.L_ab;

		assert(contains(max()));

	}

private:

	/*
	 * compact memory used by linked list F[f] and re-compute
	 * pair's indexes
	 */
	void compact_ll(itype f){

		assert(check_hash_consistency());

		assert(max_size>0);

		F[f].compact();

		for(itype i=0;i<F[f].size();++i){

			assert(not F[f][i].is_null());

			auto ab = F[f][i].ab;

			assert(contains(ab));

			H[ab] = {f,i};

			assert(H[ab].second < F[H[ab].first].capacity());

		}

		assert(check_hash_consistency());

	}

	bool check_hash_consistency(){

		//comment this line to enable the check
		//be aware that this check introduces a lot of overhead,
		//so run it only on small datasets
		return true;

		for(auto el : H){

			auto p = el.first;
			if(H[p].second >= F[H[p].first].capacity()){

				cout << "Failed pair: " << (char)p.first << (char)p.second << endl;
				cout << H[p].second << " / " << F[H[p].first].capacity() << endl;

				return false;

			}

		}

		return true;

	}

	//number of elements in the queue
	itype n = 0;

	itype max_size = 0;
	itype max_freq = 0;

	itype MAX = 0;

	vector<ll_type> F;
	hash_t H;

	const itype null = ~itype(0);
	const cpair nullpair = {null,null};

};

typedef lf_queue<ll_vec32_t> lf_queue32_t;
typedef lf_queue<ll_vec64_t> lf_queue64_t;

#endif /* INTERNAL_LF_QUEUE_HPP_ */
