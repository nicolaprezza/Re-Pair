/*
 * skippable_text.hpp
 *
 *  Created on: Jan 18, 2017
 *      Author: nico
 *
 *  class representing a text containing special skip characters.
 *  the class allows skipping runs of blank characters,
 *  accessing pairs of consecutive text characters, and replacing pairs with a
 *  symbol. All constant-time.
 *
 */

#ifndef INTERNAL_SKIPPABLE_TEXT_HPP_
#define INTERNAL_SKIPPABLE_TEXT_HPP_

#include <vector>
#include <sdsl/int_vector.hpp>

using namespace std;
using namespace sdsl;

template<typename itype = uint32_t, typename ctype = uint32_t>
class skippable_text{

	using cpair = pair<ctype,ctype>;

public:

	/*
	 * initialize new empty text (filled with charcter 0).
	 * The size of each character is max(8, bitsize(n))
	 *
	 */
	skippable_text(itype n){

		assert(n>0);

		this->n = n;

		width = 64 - __builtin_clzll(uint64_t(n));
		width = std::max<itype>(8,width);

		blank = vector<bool>(n,false);

		T = int_vector<>(n,0,width);

	}

	/*
	 * return i-th character (BLANK if i is a blank position)
	 *
	 * codomain of this function is alphabet + dictionary symbols
	 *
	 */
	ctype operator[](itype i){

		assert(i<n);

		return blank[i] ? BLANK : T[i];

	}

	bool is_blank(itype i){

		return blank[i];

	}

	/*
	 * set i-th position to character c.
	 */
	void set(itype i, ctype c){

		assert(i<n);
		T[i] = c;

	}

	/*
	 * return pair starting at position i < n-1;
	 *
	 * this position skips runs of blanks.
	 *
	 * if i contains a BLANK, the function returns <BLANK,BLANK>
	 */
	cpair pair_starting_at(itype i){

		assert(i<n-1);

		if(operator[](i)==BLANK) return {BLANK,BLANK};

		//blank[i+1] -> the position pointed to ends before the text end
		assert(not blank[i+1] || i + T[i+1] +1 < n);
		//blank[i+1] -> the position pointed to does not contain a blank
		assert(not blank[i+1] || (not blank[i + T[i+1] +1]));

		ctype first = T[i];
		ctype second = blank[i+1] ? T[ i + T[i+1] +1 ] : T[i+1];

		return {first,second};

	}

	/*
	 * return pair ending at position i > 0
	 *
	 * this position skips runs of blanks.
	 *
	 * if i contains a BLANK, the function returns <BLANK,BLANK>
	 */
	cpair pair_ending_at(itype i){

		assert(i>0);

		if(operator[](i)==BLANK) return {BLANK,BLANK};

		//blank[i-1] -> the position pointed to does not start before the text
		assert(not blank[i-1] || i  >= (T[i-1]+1));
		//blank[i-1] -> the position pointed to does not contain BLANK
		assert(not blank[i-1] || (not blank[i - (T[i-1]+1)]));

		ctype first = blank[i-1] ? T[ i - (T[i-1]+1) ] : T[i-1];
		ctype second = T[i];

		return {first,second};

	}

	/*
	 * replace pair starting at position i with symbol X
	 *
	 * internally, this function replaces AB with X_, AB being the
	 * pair starting at position i and _ being the blank character.
	 *
	 * the function automatically skips blank characters and merges blanks if needed
	 */
	void replace(itype i, ctype X){

		assert(not blank[i]);
		assert(i<n-1);

		//blank[i+1] -> the position pointed to ends before the text end
		assert(not blank[i+1] || i + T[i+1] +1 < n);
		//blank[i+1] -> the position pointed to does not contain a blank
		assert(not blank[i+1] || (not blank[i + T[i+1] +1]));

		//position of next non-blank character
		itype i_next = blank[i+1] ? i+T[i+1]+1 : i+1;

		assert(i_next >= i+1);

		//length of run of blanks starting in position i+1, if any
		itype len = i_next - (i+1);

		//length of the run following T[i_next]
		itype next_len = i_next == n-1 ? 0 : ( blank[i_next+1] ? T[i_next+1] : 0 );

		//set next char's position to blank
		blank[i_next] = true;

		itype new_len = len + next_len + 1;

		assert(new_len < n);
		assert(blank[i+new_len]);

		//store run's length in the first run position
		T[i+1] = new_len;
		//store run's length in the last run position
		T[i+new_len] = new_len;

		T[i] = X;

	}

	/*
	 * return size of the text including blnk characters
	 */
	itype size(){

		return n;

	}

	static const ctype BLANK = ~ctype(0);

private:

	/*
	 * TODO: replace T and blank with blocked compressed vectors. In this way we should get close to n bytes of usage for the text
	 * and still keep constant time access/update
	 */

	//this is the text
	int_vector<> T;

	itype n;

	//this bitvector marks blank positions
	//the first and last blank positions in a run of blank positions
	//contain the length of the run of blanks
	vector<bool> blank;

	uint8_t width;


};

typedef skippable_text<uint32_t, uint32_t> skippable_text32_t;
typedef skippable_text<uint64_t, uint64_t> skippable_text64_t;


#endif /* INTERNAL_SKIPPABLE_TEXT_HPP_ */
