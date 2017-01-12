/*
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
 *  decrease(ab): decrease by 1 ab's frequency F_ab
 *  insert(list_el), where list_el = <ab, P_ab, L_ab, F_ab> is a linked list element
 *
 *
 */

#include <ll_vec.hpp>
#include <unordered_map>
#include <ll_el.hpp>
#include <functional>

using namespace std;

#ifndef INTERNAL_HF_QUEUE_HPP_
#define INTERNAL_HF_QUEUE_HPP_

/*
 * define hash functions for pairs of (32-bits/64-bits) integers
 */
namespace std{

template <>
struct hash<pair<uint32_t,uint32_t> >{

	std::size_t operator()(const pair<uint32_t,uint32_t>& k) const{

		return hash<uint32_t>()(k.first) ^ hash<uint32_t>()(k.second);

	}

};

template <>
struct hash<pair<uint64_t,uint64_t> >{

	std::size_t operator()(const pair<uint64_t,uint64_t>& k) const{

		return hash<uint64_t>()(k.first) ^ hash<uint64_t>()(k.second);

	}

};

}

/*
 * template on linked list type and integer type
 */
template<typename ll_type = ll_vec32_t, typename itype = uint32_t, typename ctype = uint32_t>
class hf_queue{

using cpair = pair<ctype,ctype>;
using triple = std::tuple<itype, itype, itype>;
using hash_t = std::unordered_map<cpair, itype>;
using el_type = typename ll_type::el_type;

public:

	/*
	 * default constructor. Note that object must be created with the other constructor in order to be
	 * usable (using object built with this constructor causes failed assertions)
	 */
	hf_queue(){

		H = NULL;
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

	/*
	 * return triple <P_ab, L_ab, F_ab> relative to pair ab
	 * complexity: O(1)
	 */
	triple operator[](cpair ab){

		assert(max_size>0);

		auto e = B[H[ab]];
		return {e.P_ab, e.L_ab, e.F_ab};

	}

	cpair min(){

		assert(max_size>0);

		return B.min();

	}

	cpair max(){

		assert(max_size>0);

		return B.max();

	}

	void remove(cpair ab){

		assert(max_size>0);

		B.remove(H[ab]);
		H.erase(ab);

	}

	bool contains(cpair ab){

		assert(max_size>0);

		return H.count(ab) == 1;

	}

	itype size(){

		assert(max_size>0);

		return B.size();

	}

	/*
	 * decrease by 1 F_ab
	 */
	void decrease(cpair ab){

		assert(max_size>0);

		//frequency must be >0, otherwise we would alredy have removed the pair
		assert(B[H[ab]].F_ab>0);

		B[H[ab]].F_ab--;

		//if frequency becomes too small, remove pair
		if(B[H[ab]].F_ab < min_freq){

			remove(ab);

			//if more than half of B's entries are empty, compact B.
			if(B.size() < B.capacity()/2) compact_ll();

		}

	}

	void insert(el_type el){

		assert(max_size>0);

		cpair ab = el.ab;

		//must not already contain ab
		assert(not contains(ab));
		//must meet requirement on minimum frequency
		assert(el.F_ab >= min_freq);

		itype idx = B.insert(ab);

		H.insert({ab,idx});

		//must not exceed max capacity
		assert(size()<=max_size);

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

	const static itype null = ~itype(0);

};

typedef hf_queue<ll_vec32_t, uint32_t, uint32_t> hf_queue32_t;
typedef hf_queue<ll_vec64_t, uint64_t, uint64_t> hf_queue64_t;


#endif /* INTERNAL_HF_QUEUE_HPP_ */
