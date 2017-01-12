/*
 * ll_el.hpp
 *
 *  Created on: Jan 11, 2017
 *      Author: nico
 *
 *  a linked list element: quadruple <ab, P_ab, L_ab, F_ab>
 *
 */

#include <cassert>

using namespace std;

#ifndef INTERNAL_LL_EL_HPP_
#define INTERNAL_LL_EL_HPP_

/*
 * template on the char type and on the integer type
 */
template<typename ctype = uint32_t, typename itype = uint32_t>
class ll_el{

public:

	ll_el(){}

	ll_el(ctype a, ctype b){

		ab.first = a;
		ab.second = b;

		F_ab = 0;
		L_ab = 0;

	}

	bool operator<(ll_el &e1) const {
		return F_ab < e1.F_ab;
	}

	bool is_null(){
		return F_ab==null;
	}

	//this value is reserved to indicate NULL elements/pointers
	const static itype null = ~itype(0);

	pair<ctype,ctype> ab;
	itype P_ab = 0;
	itype L_ab = 0;
	itype F_ab = null;

};

typedef ll_el<uint32_t,uint32_t> ll_el32_t;
typedef ll_el<uint64_t,uint64_t> ll_el64_t;


#endif /* INTERNAL_LL_EL_HPP_ */
