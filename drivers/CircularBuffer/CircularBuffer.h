/*
  CircularBuffer - An Arduino circular buffering library for arbitrary types.

  Created by Ivo Pullens, Emmission, 2014-2016 -- www.emmission.nl

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

/**
* @file CircularBuffer.h
*
* Circular buffering for arbitrary types.
*/

#ifndef CircularBuffer_h
#define CircularBuffer_h

/**
 * The circular buffer class.
 * Pass the datatype to be stored in the buffer as template parameter.
 */
template <class T> class CircularBuffer
{
public:
	/**
	 * Constructor
	 * @param buffer   Preallocated buffer of at least size records.
	 * @param size     Number of records available in the buffer.
	 */
	CircularBuffer(T* buffer, const uint8_t size )
		: m_size(size), m_buff(buffer)
	{
		clear();
	}

	/**
	  * Clear all entries in the circular buffer.
	  */
	void clear(void)
	{
		MY_CRITICAL_SECTION {
			m_front = 0;
			m_fill  = 0;
		}
	}

	/**
	 * Test if the circular buffer is empty.
	 * @return True, when empty.
	 */
	inline bool empty(void) const
	{
		bool empty;
		MY_CRITICAL_SECTION {
			empty = !m_fill;
		}
		return empty;
	}

	/**
	 * Test if the circular buffer is full.
	 * @return True, when full.
	 */
	inline bool full(void) const
	{
		bool full;
		MY_CRITICAL_SECTION {
			full = m_fill == m_size;
		}
		return full;
	}

	/**
	 * Return the number of records stored in the buffer.
	 * @return number of records.
	 */
	inline uint8_t available(void) const
	{
		uint8_t ret_value;
		MY_CRITICAL_SECTION {
			ret_value = m_fill;
		}
		return ret_value;
	}

	/**
	 * Aquire unused record on front of the buffer, for writing.
	 * After filling the record, it has to be pushed to actually
	 * add it to the buffer.
	 * @return Pointer to record, or NULL when buffer is full.
	 */
	T* getFront(void) const
	{
		MY_CRITICAL_SECTION {
			if (!full())
			{
				return get(m_front);
			}
		}
		return static_cast<T*>(NULL);
	}

	/**
	 * Push record to front of the buffer.
	 * @param record   Record to push. If record was aquired previously (using getFront) its
	 *                 data will not be copied as it is already present in the buffer.
	 * @return True, when record was pushed successfully.
	 */
	bool pushFront(T* record)
	{
		MY_CRITICAL_SECTION {
			if (!full())
			{
				T* f = get(m_front);
				if (f != record) {
					*f = *record;
				}
				m_front = (m_front+1) % m_size;
				m_fill++;
				return true;
			}
		}
		return false;
	}

	/**
	 * Aquire record on back of the buffer, for reading.
	 * After reading the record, it has to be pop'ed to actually
	 * remove it from the buffer.
	 * @return Pointer to record, or NULL when buffer is empty.
	 */
	T* getBack(void) const
	{
		MY_CRITICAL_SECTION {
			if (!empty())
			{
				return get(back());
			}
		}
		return static_cast<T*>(NULL);
	}

	/**
	 * Remove record from back of the buffer.
	 * @return True, when record was pop'ed successfully.
	 */
	bool popBack(void)
	{
		MY_CRITICAL_SECTION {
			if (!empty())
			{
				m_fill--;
				return true;
			}
		}
		return false;
	}

protected:
	/**
	 * Internal getter for records.
	 * @param idx   Record index in buffer.
	 * @return Ptr to record.
	 */
	inline T * get(const uint8_t idx) const
	{
		return &(m_buff[idx]);
	}

	/**
	 * Internal getter for index of last used record in buffer.
	 * @return Index of last record.
	 */
	inline uint8_t back(void) const
	{
		return (m_front - m_fill + m_size) % m_size;
	}

	const uint8_t      m_size;     //!< Total number of records that can be stored in the buffer.
	T* const           m_buff;     //!< Ptr to buffer holding all records.
	volatile uint8_t   m_front;    //!< Index of front element (not pushed yet).
	volatile uint8_t   m_fill;     //!< Amount of records currently pushed.
};

#endif // CircularBuffer_h
