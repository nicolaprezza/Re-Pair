/*
 * ll_vec.hpp
 *
 *  Created on: Jan 11, 2017
 *      Author: nico
 *
 *  A doubly-linked list of elements of some type t. Memory allocation is managed internally: all elements are stored contiguously.
 *  Type t must define internally a comparator <. The list supports the following operations:
 *
 *      pop(): remove and return the first element from the list. Complexity: O(1)
 *      popmax(): remove and return the max element from the list (uses the comparator). Complexity: linear
 *      min()/max(): return pair ab with the lowest/largest frequency F_ab
 *      insert(x) insert x on top of the list. Return a pointer (integer) to x. The pointer is relative and refers
 *                to allocated slots in the list. Complexity: O(1)
 *                if there isn't enough allocated space, we increase the allocated space by 50%: alloc' <- alloc * 3/2. In this case,
 *                relative pointers to elements in the list remain unchanged.
 *      operator[i] return object at position i. return NULL if no object is stored at position i. Complexity: O(1)
 *      remove(i) remove object at position i. Position i will contain NULL. Complexity: O(1)
 *      alloc() return number of allocated slots. Complexity: O(1)
 *      size() return number of slots actually filled with objects. Complexity: O(1)
 *      compact(): remove all NULL positions. Note that pointers to list's objects
 *                 change after this operation, so a scan is needed to retrieve the new pointers.
 *
 */

#include <vector>
#include "stdint.h"
#include <ll_el.hpp>
#include <cassert>

using namespace std;

#ifndef INTERNAL_LL_VEC_HPP_
#define INTERNAL_LL_VEC_HPP_

template<typename t = ll_el32_t, typename itype = uint32_t, typename ctype = uint32_t>
class ll_vec{

using cpair = pair<ctype,ctype>;

public:

	using el_type = t;

	/*
	 * constructor: initialize empty list, allocate memory for 1 element.
	 */
	ll_vec(){

		V = vector<t>(1);

		next_el = vector<itype>(1);
		prev_el = vector<itype>(1);

		next_el[0] = null;
		prev_el[0] = null;

		first_el = null;
		first_empty = 0;

		n = 0;

	}


	/*
	 * return reference to element stored at position i (could be null
	 */
	t & operator[](itype i){

		assert(i < capacity());
		return V[i];

	}

	/*
	 * extract head of the list and remove it from the set
	 */
	t pop(){

		if(size()==0) return t();

		assert(size()>0);
		assert(first_el != null);

		assert(not V[first_el].is_null());

		//we are popping the first element: save its position in temp variable
		itype this_pos = first_el;

		//read first element
		t el = V[first_el];
		//move pointer to the next element (could be null)
		first_el = next_el[first_el];

		//this position is empty, so now it points to next empty position

		assert(this_pos<next_el.size());
		next_el[this_pos] = first_empty;
		//first empty position becomes this position
		first_empty = this_pos;

		//decrease size
		n--;

		return el;

	}

	/*
	 * linear-scan the array and pop element with largest value
	 */
	t popmax(){

		assert(size()>0);
		assert(first_el != null);

		itype prev = null;
		itype curr = first_el;
		itype next = next_el[curr];

		//maximum is the first at the beginning
		itype prev_m = prev;
		itype curr_m = curr;
		itype next_m = next;

		while(next != null){

			assert(not V[curr].is_null());

			//jump to next element
			prev = curr;
			curr = next;
			next = next_el[next];

			if(V[curr_m] < V[curr]){

				//new max

				prev_m = prev;
				curr_m = curr;
				next_m = next;

			}

		}

		//this is the maximum element
		t el = V[curr_m];

		//erase element
		V[curr_m] = t();

		//this position is now empty, so now it points to next empty position
		next_el[curr_m] = first_empty;
		//first empty position becomes this position
		first_empty = curr_m;

		//now we have to remove curr_m from the list of non-empty positions: link prev_m -> next_m

		if(prev_m == null){

			//in this case, curr_m was already the first element
			assert(first_el == curr_m);

			first_el = next_m;

		}else{

			//curr_m has a predecessor
			next_el[prev_m] = next_m;

			//curr_m might not have a successor
			if(next_m != null)
				prev_el[next_m] = prev_m;

		}

		//decrease size
		n--;

		return el;

	}

	/*
	 * linear-scan the array and retrieve pair with smallest F_ab
	 */
	t min(){

		assert(size()>0);
		assert(first_el != null);

		itype curr = first_el;
		itype next = next_el[curr];

		//minimum is the first at the beginning
		itype curr_m = curr;
		itype next_m = next;

		while(next != null){

			assert(not V[curr].is_null());

			//jump to next element
			curr = next;
			next = next_el[next];

			if(V[curr] < V[curr_m]){

				//new max

				curr_m = curr;
				next_m = next;

			}

		}

		//this is the min element
		t el = V[curr_m];

		return el.ab;

	}

	/*
	 * linear-scan the array and retrieve pair with largest F_ab
	 */
	t max(){

		assert(size()>0);
		assert(first_el != null);

		itype curr = first_el;
		itype next = next_el[curr];

		//minimum is the first at the beginning
		itype curr_m = curr;
		itype next_m = next;

		while(next != null){

			assert(not V[curr].is_null());

			//jump to next element
			curr = next;
			next = next_el[next];

			if(V[curr] > V[curr_m]){

				//new max

				curr_m = curr;
				next_m = next;

			}

		}

		//this is the min element
		t el = V[curr_m];

		return el.ab;

	}

	/*
	 * remove element at position i. note: i must not contain null
	 */
	void remove(itype i){

		assert(i<V.size());
		assert(size()>0);
		assert(not V[i].is_null());

		//insert null in this position
		V[i] = t();

		auto prev = prev_el[i]; //save temporarily previous element
		auto next = next_el[i]; //save temporarily next element

		//this position is now empty, so now it points to next empty position
		next_el[i] = first_empty;
		//first empty position becomes this position
		first_empty = i;

		if(prev == null){

			//in this case, i was the first element
			assert(first_el == i);

			first_el = next;

		}else{

			//i has a predecessor
			next_el[prev] = next;

			//i might not have a successor
			if(next != null)
				prev_el[next] = prev;

		}

		//decrease size;
		n--;

	}

	/*
	 * insert new element x. return (relative) pointer to position where x is stored.
	 */
	itype insert(t x){

		if(first_empty == null){ //if there is no space left

			//V must be full
			assert(size() == V.size());

			//re-allocate memory

			itype old_size = n;

			//increase size at least by 1
			itype new_size = n + (n/2==0?1:n/2);

			V.resize(new_size);
			V.shrink_to_fit();

			first_empty = old_size;

			next_el.resize(new_size);
			next_el.shrink_to_fit();

			prev_el.resize(new_size);
			prev_el.shrink_to_fit();

			for(itype i = old_size;i<new_size;++i){

				next_el[i] = i+1 == new_size ? null : i+1;
				prev_el[i] = i == old_size ? null : i-1;

			}

		}

		//now there is enough space to insert elements

		//modify pointers of empty elements
		auto insert_pos = first_empty;
		first_empty = next_el[insert_pos];

		assert(V[insert_pos].is_null());

		//insert x
		V[insert_pos] = x;

		//modify pointers of full positions
		next_el[insert_pos] = first_el;
		prev_el[insert_pos] = null;

		if(first_el != null)
			prev_el[first_el] = insert_pos;

		first_el = insert_pos;

		n++;

		return insert_pos;

	}

	/*
	 * return number of elements stored in the list
	 */
	itype size(){

		return n;

	}

	/*
	 * return number of allocated slots
	 */
	itype capacity(){

		assert(V.capacity() == V.size());

		return V.size();

	}

	/*
	 * remove null positions, re-allocate memory. If size() == 0, leaves only 1 empty position.
	 */
	void compact(){

		if(n==0){

			V = vector<t>(1);

			next_el = vector<itype>(1);
			prev_el = vector<itype>(1);

			next_el[0] = null;
			prev_el[0] = null;

			first_el = null;
			first_empty = 0;

			n = 0;

		}else{

			{
				vector<t> tmp(n);

				auto cur_pos = first_el;

				itype i = 0;

				while(cur_pos != null){

					assert(i<n);

					tmp[i++] = V[cur_pos];
					cur_pos = next_el[cur_pos];

				}

				assert(i==n);

				V = vector<t>(tmp);

			}

			first_empty = null;
			first_el = 0;

			//shrink capacities of pointer vectors

			next_el.resize(n);
			std::vector<itype>(next_el).swap(next_el);

			prev_el.resize(n);
			std::vector<itype>(prev_el).swap(prev_el);

			for(itype i=0;i<n;++i){

				prev_el[i] = i==0 ? null : i-1;
				next_el[i] = i==n-1 ? null : i+1;

			}

		}

		assert(capacity()==size());

	}

private:

	vector<t> V;

	//elements are doubly chained: this permits to retrieve in constant time
	//empty spaces and elements from the set
	vector<itype> next_el;
	vector<itype> prev_el; //not used for null positions

	//this value is reserved to indicate NULL elements/pointers
	const static itype null = ~itype(0);

	//number of elements actually stored
	itype n;

	itype first_el;
	itype first_empty;

};

typedef ll_vec<ll_el32_t,uint32_t,uint32_t> ll_vec32_t;
typedef ll_vec<ll_el64_t,uint64_t,uint32_t> ll_vec64_t;


#endif /* INTERNAL_LL_VEC_HPP_ */
