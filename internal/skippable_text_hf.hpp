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
 * skippable_text_hf.hpp
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

#ifndef INTERNAL_SKIPPABLE_TEXT_HF_HPP_
#define INTERNAL_SKIPPABLE_TEXT_HF_HPP_

#include <vector>

using namespace std;

template<typename itype = uint32_t, typename ctype = uint32_t>
class skippable_text_hf{

public:

	using char_type = ctype;
	using cpair = pair<ctype,ctype>;

	/*
	 * initialize new empty text (filled with charcter 0).
	 * The size of each character is max(8, bitsize(n))
	 *
	 */
	skippable_text_hf(itype n, ctype largest_symbol){

		assert(n>0);

		this->n = n;
		non_blank_characters = n;

		eof = largest_symbol+1;

		width = 64 - clz(eof);

		assert(width > 0);

		non_blank = vector<uint64_t>(n/64+(n%64!=0),~uint64_t(0));//init all '1'

		if(n%64 != 0){//set to 0 bits in the right padding

			uint64_t MASK = ~((~uint64_t(0)) >> (n%64));
			non_blank[non_blank.size()-1] &= MASK;

		}

		//vector storing length of skips. Init with all 0
		skips = vector<uint64_t>(n/64+(n%64!=0),0);

		//T = int_vector<>(n, 0, width);
		T = vector<uint16_t>(n, 0); width = 16;

	}

	/*
	 * return i-th character (BLANK if i is a blank position)
	 *
	 * codomain of this function is alphabet + dictionary symbols
	 *
	 */
	ctype operator[](itype i){

		assert(i<n);

		return is_blank(i) ? BLANK : T[i];

	}

	bool is_blank(itype i){

		return not ((non_blank[i/64] >> (63-(i%64))) & uint64_t(1));

	}

	/*
	 * set i-th position to character c != BLANK
	 */
	void set(itype i, ctype c){

		assert(c != BLANK);
		assert(i<n);

		T[i] = c;

	}

	/*
	 * return pair starting at position i < n;
	 *
	 * this position skips runs of blanks in constant time.
	 *
	 * if i is last text's character or i is a blank position, return blank pair
	 *
	 */
	cpair pair_starting_at(itype i){

		assert(i<T.size());

		cpair result = (i == n-1 or is_blank(i)) ? blank_pair() :
						(T[i+1] == eof ? blank_pair() : cpair(T[i],T[i+1]));

		//check that we get the same results by jumping or reading next character
		assert(result == pair_starting_at_1(i));

		return result;

	}



	/*
	 * return pair that follows pair starting at position i
	 *
	 * this position skips runs of blanks.
	 *
	 * return blank pair if there is no next pair or if position i contains a blank
	 *
	 */
	cpair next_pair(itype i){

		assert(not is_blank(i));
		assert(i<T.size());

		itype i_1 = next_non_blank_position(i);

		return i_1 == null ? blank_pair() : pair_starting_at(i_1);

	}


	/*
	 * return pair ending at position i > 0
	 *
	 * this position skips runs of blanks.
	 *
	 * if i is first text's character or i is a non_blank position, return blank pair
	 */
	cpair pair_ending_at(itype i){

		return pair_ending_at_1(i);

		assert(i<T.size());

		cpair result;

		if(i<2 || (is_blank(i-1) && (not is_blank(i-2)))){

			result = pair_ending_at_1(i);

		}else{

			//i >= 2 and (not_blank(i-1) OR blank(i-2))
			result = is_blank(i) ? blank_pair() : cpair(T[i-1],T[i]);

		}

		//check that we get the same results by jumping or reading next character
		assert(result == pair_ending_at_1(i));

		return result;

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
	 * the function automatically skips non_blank characters and merges blanks if needed
	 */
	void replace(itype i, ctype X){

		assert(64 - clz(uint64_t(X)) <= width);
		assert(i<n-1);
		assert(not is_blank(i));

		itype i2 = next_non_blank_position(i);

		//there is a pair starting from position i
		assert(i2 != null);

		assert(T[i+1] == T[i2]);

		itype i3 = next_non_blank_position(i2);

		itype b1 = i/64;
		itype b2 = i2/64;
		itype b3 = i3/64;

		//set to 0 the bit under position i2 in not_blank
		uint64_t MASK = ~(uint64_t(1) << (63-(i2%64)));//11111110111...1, with a 0 at position i2%64
		non_blank[b2] &= MASK;

		assert(non_blank_characters>0);
		non_blank_characters--;

		if(i3 != null and b3 > b1+1){

			//case 1: there is at least 1 block between i and i3: explicitly store skip
			//length in block following b1 and in block preceding b3

			//skip length
			itype skip = (i3-i)-1;

			//store skip lengths

			assert(non_blank[b1+1] == 0);
			assert(non_blank[b3-1] == 0);

			skips[b1+1] = skip;
			skips[b3-1] = skip;

		}

		if(i3 == null){

			//case 2: i3 does not exist: pair starting at i is the last one

			//if block b1 is either last or the one before last, no need to store explicitly skip length.
			//otherwise we need to do it
			if(b1 < non_blank.size() - 2){

				assert(non_blank[b1+1] == 0);

				skips[b1+1] = (T.size() - i)-1;

			}

		}

		//replace T[i] with X
		T[i] = X;

		assert(is_blank(i+1));

		//store in T[i+1] the next character: this allows avoiding
		//jumping inside function pair_starting_at
		T[i+1] = i3 == null ? eof : T[i3];

		//fix also previous non-blank position
		itype i0 = prev_non_blank_position(i);
		if(i0 != null){

			assert(i0+1<n);
			T[i0+1] = X;

		}

		if(i3 != null && i3>1 && is_blank(i3-2)){

			assert(i3>0);
			assert(is_blank(i3-1));
			T[i3-1] = X;

		}

	}

	/*
	 * return size of the text including blank characters
	 */
	itype size(){

		return n;

	}

	/*
	 * return number of characters different than non_blank
	 */
	itype number_of_non_blank_characters(){
		return non_blank_characters;
	}

private:

	uint64_t clz(uint64_t x){

		return x == 0 ? 64 : __builtin_clzll(x);

	}

	uint64_t ctz(uint64_t x){

		return x == 0 ? 64 : __builtin_ctzll(x);

	}

	cpair pair_starting_at_1(itype i){

		assert(i<T.size());

		return((is_blank(i) ? null : next_non_blank_position(i)) == null ?
				blank_pair() : cpair {T[i],T[(is_blank(i) ? null : next_non_blank_position(i))]});

	}

	cpair pair_ending_at_1(itype i){

		assert(i<T.size());

		itype i_1 = is_blank(i) ? null : prev_non_blank_position(i);

		return i_1 == null ? blank_pair() : cpair {T[i_1],T[i]};

	}

	/*
	 * input: a non-blank position i
	 * output: next non-blank position.
	 * if i is the last blank position, returns null
	 *
	 */
	itype next_non_blank_position(itype i){

		assert(i<T.size());
		assert(not is_blank(i));

		itype result = null;

		itype block = i/64;
		itype off = i%64;

		uint64_t MASK = off == 63 ? 0 : (~uint64_t(0)) >> (off+1);

		itype next_off = off == 63 ? 64 : clz(non_blank[block] & MASK);

		if(next_off < 64){

			result = block*64 + next_off;

			assert(result > i);

		}else{

			//case 2: next non-blank character is not within this block

			//case 2.1: if this is last block, there isn't a next non-non_blank character
			if(block != non_blank.size()-1){//if ==, result is null

				assert(block+1 < non_blank.size());

				if(non_blank[block+1] != 0){

					//case 2.2: next block contains a non-blank character

					next_off = clz(non_blank[block+1]);

					result = (block+1)*64 + next_off;

					assert(result<T.size());
					assert(result > i);

				}else{

					if(block+1 == non_blank.size()-1){

						//if block+1 == non_blank.size()-1 then there is no pair starting at position i
						assert(non_blank[block+1] == 0);

					}else{

						assert(non_blank[block+1] == 0);

						//next block contains only blanks: then, skips[block+1] contains the skip length
						itype skip_len = skips[block+1];

						itype i_1 = i + skip_len + 1;

						//if i_1 is within text boundaries, then it cannot be blank
						assert(i_1 >= T.size() || not is_blank(i_1));

						//i_1 could go beyond text if T[i] is last non-blank text character
						result = i_1<T.size() ? i_1 : null;

						assert(result > i);

					}

				}

			}

		}

		return result;

	}

	/*
	 * trivial linear-time implementation for debugging
	 */
	itype next_non_blank_position_trivial(itype i){

		assert(not is_blank(i));

		if(i==T.size()-1) return null;

		itype j = i+1;

		while(j < T.size() && is_blank(j)){j++;}

		return j == T.size() ? null : j;

	}


	/*
	 * input: a non-blank position i
	 * output: previous non-blank position.
	 * if i is the first blank position, returns null
	 *
	 */
	itype prev_non_blank_position(itype i){

		assert(i<T.size());
		assert(not is_blank(i));

		itype block = i/64;
		itype off = i%64;

		uint64_t prev_bits = off == 0 ? 0 : (non_blank[block] >> (64-off));

		if(prev_bits != 0){

			//case 1: there is a non-blank position before position i inside this block

			assert(i >= (ctz( prev_bits )+1));

			itype i_1 = i - (ctz( prev_bits )+1);

			assert(not is_blank(i_1));

			return i_1;

		}

		//case 2: there isn't a non-blank position before position i inside this block

		if(block == 0) return null; //this is the first block: i is the first non-blank position

		if(non_blank[block-1] != 0){

			//case 2.1: there is a non-blank position in the previous block

			itype tz = ctz( non_blank[block-1] );

			assert(tz<64);

			itype i_1 = block*64 - (tz+1);

			assert(not is_blank(i_1));

			return i_1;

		}

		//case 2.2: there isn't a non-blank position in the previous block

		if(block-1 == 0) return null; //previous block is the first block: i is the first non-blank position

		itype skip_len = skips[block-1];

		assert(i < (skip_len+1) || not is_blank(i - (skip_len+1)));

		return i >= (skip_len+1) ? i - (skip_len+1) : null;

	}


	const ctype BLANK = ~ctype(0);
	const ctype null = ~itype(0);

	//this is the text

	//int_vector<> T;
	vector<uint16_t> T;

	itype n = 0;
	itype non_blank_characters = 0;

	//this bitvector marks non_blank positions
	//the first and last blank positions in a run of non_blank positions
	//contain the length of the run of blanks
	vector<uint64_t> non_blank;

	//stores skip lengths
	vector<uint64_t> skips;

	uint8_t width = 0;

	uint64_t eof = 0;

};

typedef skippable_text_hf<uint32_t, uint32_t> skippable_text_hf32_t;
typedef skippable_text_hf<uint64_t, uint64_t> skippable_text_hf64_t;


#endif /* INTERNAL_SKIPPABLE_TEXT_HF_HPP_ */
