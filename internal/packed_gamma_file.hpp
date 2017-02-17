/*
 * packed_gamma_file.hpp
 *
 *  Created on: Feb 15, 2017
 *      Author: nico
 *
 *  read/write integers in packed form in/from a file
 *
 */

#ifndef INTERNAL_PACKED_GAMMA_FILE_HPP_
#define INTERNAL_PACKED_GAMMA_FILE_HPP_

#include <cassert>
#include <vector>
#include <fstream>

using namespace std;

#endif /* INTERNAL_PACKED_GAMMA_FILE_HPP_ */

template<uint64_t block_size = 10>
class packed_gamma_file{

public:

	/*
	 * constructor
	 *
	 * input: file name and flag indicating whether file is used in write mode (true) or read mode (false)
	 *
	 */
	packed_gamma_file(string filename, bool write = true){

		this->write = write;

		if(write){

			out = ofstream(filename, std::ios::binary);

		}else{

			in = ifstream(filename, std::ios::binary);

		}

		lower_bound_bitsize = 0;
		actual_bitsize = 0;

	}

	/*
	 * append integer x in packed form to the file
	 */
	void push_back(uint64_t x){

		assert(write);

		if(buffer.size() == block_size){

			//buffer full: flush to file

			flush_buffer();

		}

		buffer.push_back(x);

		lower_bound_bitsize += wd(x);

	}

	/*
	 * read next integer from file.
	 * After EOF is reached, returns always 0
	 */
	uint64_t read(){

		assert(not write);

		return 0;


	}

	/*
	 * return true if there are no more characters to be read from the file
	 */
	bool eof(){

		assert(not write);
		return 0;

	}

	/*
	 * close file: must be called when there are no more integers to be written
	 */
	void close(){

		assert(write);
		assert(buffer.size()<=block_size);

		//remaining integers that need to be written (possibly 0)
		uint64_t remaining = buffer.size();

		uint64_t END = 65;//this denotes the end of file (larger than bitsize(uint64_t))

		//push back EOF code
		for(bool b : gamma(END)) bits.push_back(b);

		//push back 64 bits with size of buffer
		for(bool b : binary(remaining,64)) bits.push_back(b);

		//push back remaining integers in buffer (using 64 bits each)
		for(uint64_t x : buffer)
			for(bool b : binary(x,64))
				bits.push_back(b);

		//pad bits with 0s until we reach a multiple of 8
		while(bits.size()%8 != 0) bits.push_back(false);

		flush_bits();

		assert(bits.size()==0);

	}

	uint64_t written_bytes(){

		return actual_bitsize/8;

	}

	uint64_t lower_bound_bytes(){

		return lower_bound_bitsize/8;

	}

	/*
	 * return % of overhead of the number of bits saved to file w.r.t.
	 * the cumulated bitlengths of the stored integers
	 */
	double overhead(){

		return 100*double(actual_bitsize-lower_bound_bitsize)/lower_bound_bitsize;

	}


private:

	void flush_buffer(){

		assert(buffer.size() == block_size);

		uint64_t w = 0;

		//detect max width of integers in buffer
		for(auto x:buffer) w = std::max(w,wd(x));

		//push back gamma code of w
		for(bool b : gamma(w)) bits.push_back(b);

		//push back w-bits codes of integers in buffer
		for(auto x:buffer)
			for(auto b:binary(x,w))
				bits.push_back(b);

		flush_bits();

		//empty buffer
		buffer = vector<uint64_t>();

	}

	//flush bits to file (bits.size()%8 bits remain in the array after this call)
	void flush_bits(){

		int n = bits.size();
		int i = 0;
		uint8_t x = 0;

		for(auto b : bits){

			x |= (uint8_t(b) << (7-i));

			if(i==7){ //we just pushed last bit of a Byte

				out.write((char*)&x,1);
				x = 0;

				actual_bitsize += 8;

			}

			i = (i+1)%8;

		}

		auto bits1 = vector<bool>();

		//move remainder at the beginning of bits
		for(int i = (n/8)*8; i<n;++i) bits1.push_back(bits[i]);

		bits = bits1;

	}

	/*
	 * bit-width of x
	 */
	uint64_t wd(uint64_t x){

		auto w = 64 - __builtin_clzll(uint64_t(x));

		return x == 0 ? 1 : w;

	}

	/*
	 * return gamma encoding of x
	 */
	vector<bool> gamma(uint64_t x){

		assert(x>0);

		auto w = wd(x);

		vector<bool> code;

		//append w-1 zeros
		for(int i=0;i<w-1;++i) code.push_back(false);

		//append binary code of x using w bits
		for(bool b : binary(x,w)) code.push_back(b);

		return code;

	}

	/*
	 * return w-bits binary code of x. If w is not specified, the bitsize of x is used.
	 */
	vector<bool> binary(uint64_t x, uint64_t w = 0){

		assert(w==0 || w>= wd(x));

		w = w == 0 ? wd(x) : w;

		vector<bool> code;

		for(int i=0;i<w;++i) code.push_back( (x>>((w-i)-1)) & uint64_t(1) );

		return code;

	}

	bool write;

	vector<bool> bits;//bits to be flushed to file

	vector<uint64_t> buffer;
	uint64_t idx_in_buf = 0;//current index in buffer while reading
	uint64_t current_wdth = 0;//current width while reading/writing from buffer

	ifstream in;
	ofstream out;

	bool end_of_file = false;

	/*
	 * stores accumulated bitlength of all integers stored in the file
	 */
	uint64_t lower_bound_bitsize = 0;

	/*
	 * stores accumulated bitlength of all integers stored in the file
	 */
	uint64_t actual_bitsize = 0;

};