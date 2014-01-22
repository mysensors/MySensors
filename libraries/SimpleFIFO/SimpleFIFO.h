#ifndef SimpleFIFO_h
#define SimpleFIFO_h
/*
||
|| @file 		SimpleFIFO.h
|| @version 	1.2
|| @author 	Alexander Brevig
|| @contact 	alexanderbrevig@gmail.com
||
|| @description
|| | A simple FIFO class, mostly for primitive types but can be used with classes if assignment to int is allowed
|| | This FIFO is not dynamic, so be sure to choose an appropriate size for it
|| #
||
|| @license
|| | Copyright (c) 2010 Alexander Brevig
|| | This library is free software; you can redistribute it and/or
|| | modify it under the terms of the GNU Lesser General Public
|| | License as published by the Free Software Foundation; version
|| | 2.1 of the License.
|| |
|| | This library is distributed in the hope that it will be useful,
|| | but WITHOUT ANY WARRANTY; without even the implied warranty of
|| | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
|| | Lesser General Public License for more details.
|| |
|| | You should have received a copy of the GNU Lesser General Public
|| | License along with this library; if not, write to the Free Software
|| | Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
|| #
||
*/
template<typename T, int rawSize>
class SimpleFIFO {
public:
	const char size;				//speculative feature, in case it's needed

	SimpleFIFO();

	T dequeue();				//get next element
	bool enqueue( T element );	//add an element
	T peek() const;				//get the next element without releasing it from the FIFO
	void flush();				//[1.1] reset to default state 

	//how many elements are currently in the FIFO?
	char count() { return numberOfElements; }

private:
#ifndef SimpleFIFO_NONVOLATILE
	volatile char numberOfElements;
	volatile char nextIn;
	volatile char nextOut;
	volatile T raw[rawSize];
#else
	char numberOfElements;
	char nextIn;
	char nextOut;
	T raw[rawSize];
#endif
};

template<typename T, int rawSize>
SimpleFIFO<T,rawSize>::SimpleFIFO() : size(rawSize) {
	flush();
}
template<typename T, int rawSize>
bool SimpleFIFO<T,rawSize>::enqueue( T element ) {
	if ( count() >= rawSize ) { return false; }
	numberOfElements++;
	nextIn %= size;
	raw[nextIn] = element;
	nextIn++; //advance to next index
	return true;
}
template<typename T, int rawSize>
T SimpleFIFO<T,rawSize>::dequeue() {
	numberOfElements--;
	nextOut %= size;
	return raw[ nextOut++];
}
template<typename T, int rawSize>
T SimpleFIFO<T,rawSize>::peek() const {
	return raw[ nextOut % size];
}
template<typename T, int rawSize>
void SimpleFIFO<T,rawSize>::flush() {
	nextIn = nextOut = numberOfElements = 0;
}
#endif
