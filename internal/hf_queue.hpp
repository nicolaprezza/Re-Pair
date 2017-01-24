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

	/*
	 * return triple <P_ab, L_ab, F_ab> relative to pair ab
	 * complexity: O(1)
	 */
	triple_t operator[](cpair ab){

		assert(max_size>0);
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

		refresh_min_and_max();

		return MIN;

	}

	/*
	 * return pair with maximum frequency
	 * if the maximum is not synchronized with the content of the queue, re-compute it.
	 */
	cpair max(){

		assert(max_size>0);

		refresh_min_and_max();

		return MAX;

	}

	void remove(cpair ab){

		refresh_min_and_max();

		assert(contains(ab));
		assert(max_size>0);
		assert(H[ab] != null);

		B.remove(H[ab]);
		H.erase(ab);

		//if more than half of B's entries are empty, compact B.
		if(B.size() < B.capacity()/2) compact_ll();

		refresh_min_and_max();

	}

	bool contains(cpair ab){

		assert(max_size>0);

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
	 * decrease by 1 F_ab
	 */
	void decrease(cpair ab){

		assert(contains(ab));
		assert(H[ab] != null);
		assert(max_size>0);

		//frequency must be >0, otherwise we would already have removed the pair
		assert(B[H[ab]].F_ab>0);

		B[H[ab]].F_ab--;

		//if frequency becomes too small, remove pair
		if(B[H[ab]].F_ab < min_freq){

			remove(ab); //this automatically re-computes minimum/max if ab was the minimum/max

		}else{

			refresh_min_and_max();

			//if the new frequency is lower than that of the minimum, this is the new minimum
			if(operator[](ab).F_ab < operator[](MIN).F_ab){

				MIN = ab;

			}

		}

	}

	void insert(el_type el){

		assert(max_size>0);

		cpair ab = el.ab;

		assert(not contains(ab));

		//must meet requirement on minimum frequency
		assert(el.F_ab >= min_freq);

		itype idx = B.insert(el);
		H.insert({ab,idx});

		//if MIN/MAX have not yet been computed, compute them
		refresh_min_and_max();

		//if the inserted element's frequency is lower than that of the minimum, this is the new minimum
		if(el.F_ab < operator[](MIN).F_ab){

			MIN = ab;

		}

		/*
		 * if the inserted element's frequency is larger than that of the max, this is the new max
		 */
		if(el.F_ab > operator[](MAX).F_ab){

			MAX = ab;

		}

		assert(H[ab]==idx);
		assert(B[H[ab]].P_ab == el.P_ab);
		assert(B[H[ab]].L_ab == el.L_ab);
		assert(B[H[ab]].F_ab == el.F_ab);
		assert(size()<=max_size);
		assert(contains(ab));

	}

private:

	void refresh_min_and_max(){

		if(MIN == cpair {null,null} or not contains(MIN))
			MIN = B.min_pair();

		if(MAX == cpair {null,null} or not contains(MAX))
			MAX = B.max_pair();

	}

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

	cpair MIN = {null,null};
	cpair MAX = {null,null};


};

typedef hf_queue<ll_vec32_t> hf_queue32_t;
typedef hf_queue<ll_vec64_t> hf_queue64_t;


#endif /* INTERNAL_HF_QUEUE_HPP_ */
