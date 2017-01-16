/*
 * lf_queue.hpp
 *
 *  Created on: Jan 16, 2017
 *      Author: nico
 *
 *  Low-frequency pairs queue
 *
 *  This queue is a triple Q = <F,H,B> of structures, where:
 *
 *  . F: is a doubly-linked frequency vector indexing all possible pair frequencies smaller than
 *    a pre-defoined quantity
 *  - H: sigma x sigma -> int is a hash table pointing at elements in B
 *  - B is a set of linked list storing all high-frequency pairs. The set contains a linked list for each
 *    distinct frequency of pairs stored in the queue
 *
 *  Supported operations (all amortized constant time)
 *
 *  operator[ab]: return triple <P_ab, L_ab, F_ab> relative to pair ab
 *  max()/min()/head(): return pair ab with max/min F_ab or head pair in the list
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

template<typename ll_type = ll_vec32_t>
struct f_vec_el{

	//linked list's pointers
	uint64_t prev;
	uint64_t next;

	//coordinates of the pair in B
	ll_type* list; //pointer to the list containing the pair
	uint64_t offset; //offset of the pair inside its list

};

/*
 * template on linked list type and integer type
 */
template<typename ll_type = ll_vec32_t, typename itype = uint32_t, typename ctype = uint32_t>
class lf_queue{

using cpair = pair<ctype,ctype>;
using ipair = pair<itype,itype>;

using hash_t = std::unordered_map<cpair, ipair>;

public:

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

	/*
	 * build queue with max capacity equal to max_size and minimum allowed
	 * frequency of a pair equal to min_frequency (included). If a pair'sfrequency
	 * becomes strictly smaller than min_frequency, then the pair is removed from the queue
	 */
	lf_queue(itype max_size, itype min_freq) {

		assert(min_freq>1);

		this->max_freq = min_freq;
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


	}

	cpair min(){

		assert(max_size>0);



	}

	cpair max(){

		assert(max_size>0);



	}

	cpair head(){

		assert(max_size>0);



	}

	void remove(cpair ab){

		assert(contains(ab));
		assert(max_size>0);



	}

	bool contains(cpair ab){

		assert(max_size>0);



	}

	itype size(){

		assert(max_size>0);

	}

	/*
	 * decrease by 1 F_ab
	 */
	void decrease(cpair ab){

		assert(contains(ab));
		assert(max_size>0);

		//frequency must be >0, otherwise we would alredy have removed the pair
		assert(B[H[ab]].F_ab>0);



	}

	void insert(el_type el){

		assert(max_size>0);



	}

private:


	itype max_size;
	itype max_freq;

	ll_type B;
	hash_t H;

	const static itype null = ~itype(0);

};

typedef hf_queue<ll_vec32_t, uint32_t, uint32_t> hf_queue32_t;
typedef hf_queue<ll_vec64_t, uint64_t, uint64_t> hf_queue64_t;

#endif /* INTERNAL_LF_QUEUE_HPP_ */
