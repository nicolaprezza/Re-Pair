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

public:

	using char_type = ctype;
	using cpair = pair<ctype,ctype>;

	/*
	 * initialize new empty text (filled with charcter 0).
	 * The size of each character is max(8, bitsize(n))
	 *
	 */
	skippable_text(itype n){

		assert(n>0);

		this->n = n;
		non_blank_characters = n;

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

		assert(c != blank);
		assert(i<n);
		T[i] = c;

	}

	/*
	 * return pair starting at position i < n-1;
	 *
	 * this position skips runs of blanks.
	 *
	 * if i is last text's character or i is a blank position, return blank pair
	 *
	 */
	cpair pair_starting_at(itype i){

		assert(i<n);

		//in this case T[i] is last text's character: no pairs starting at position i. Return blank pair.
		if( i==n-1 || (blank[i+1] and i + T[i+1] +1 >= n)) return {BLANK,BLANK};

		//if i contains blank, return blank pair
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
	 * return pair that follows pair starting at position i
	 *
	 * this position skips runs of blanks.
	 *
	 * return blank pair if there is no next pair
	 *
	 */
	cpair next_pair(itype i){

		assert(i<n);

		//in this case T[i] is last text's character: no pairs starting at position i. Return blank pair.
		if( i==n-1 || (blank[i+1] and i + T[i+1] +1 >= n)) return {BLANK,BLANK};

		//if i contains blank, return blank pair
		if(operator[](i)==BLANK) return {BLANK,BLANK};

		//blank[i+1] -> the position pointed to ends before the text end
		assert(not blank[i+1] || i + T[i+1] +1 < n);
		//blank[i+1] -> the position pointed to does not contain a blank
		assert(not blank[i+1] || (not blank[i + T[i+1] +1]));

		//next non-blank position
		itype next_pos = blank[i+1] ? i + T[i+1] +1 : i+1;

		return pair_starting_at(next_pos);

	}


	/*
	 * return pair ending at position i > 0
	 *
	 * this position skips runs of blanks.
	 *
	 * if i is first text's character or i is a blank position, return blank pair
	 */
	cpair pair_ending_at(itype i){

		assert(i>=0);

		//if i is first text's character return blank pair
		if(i==0 || (blank[i-1] and i < (T[i-1]+1))) return {BLANK,BLANK};

		if(operator[](i)==BLANK) return {BLANK,BLANK};

		//blank[i-1] -> the position pointed to does not start before the text
		assert(not blank[i-1] || i  >= (T[i-1]+1));
		//blank[i-1] -> the position pointed to does not contain BLANK
		assert(not blank[i-1] || (not blank[i - (T[i-1]+1)]));

		ctype first = blank[i-1] ? T[ i - (T[i-1]+1) ] : T[i-1];
		ctype second = T[i];

		return {first,second};

	}

	cpair blank_pair(){
		return {BLANK,BLANK};
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

		assert(non_blank_characters>0);
		non_blank_characters--;

	}

	/*
	 * return size of the text including blank characters
	 */
	itype size(){

		return n;

	}

	/*
	 * return number of characters different than blank
	 */
	itype number_of_non_blank_characters(){
		return non_blank_characters;
	}

private:

	const ctype BLANK = ~ctype(0);

	/*
	 * TODO: replace T and blank with blocked compressed vectors. In this way we should get close to n bytes of usage for the text
	 * and still keep constant time access/update
	 */

	//this is the text
	int_vector<> T;

	itype n = 0;

	//this bitvector marks blank positions
	//the first and last blank positions in a run of blank positions
	//contain the length of the run of blanks
	vector<bool> blank;

	itype non_blank_characters = 0;

	uint8_t width = 0;


};

typedef skippable_text<uint32_t, uint32_t> skippable_text32_t;
typedef skippable_text<uint64_t, uint64_t> skippable_text64_t;


#endif /* INTERNAL_SKIPPABLE_TEXT_HPP_ */
