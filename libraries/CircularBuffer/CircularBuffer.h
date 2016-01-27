/*
  CircularBuffer - An Arduino circular buffering library for arbitrary types.

  Created by Ivo Pullens, Emmission, 2014 -- www.emmission.nl
  
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

#ifndef CircularBuffer_h
#define CircularBuffer_h

#define DISABLE_IRQ       \
  uint8_t sreg = SREG;    \
  cli();

#define RESTORE_IRQ        \
  SREG = sreg;

template <class T> class CircularBuffer
{
  public:
    /** Constructor
     * @param buffer   Preallocated buffer of at least size records.
     * @param size     Number of records available in the buffer.
     */
    CircularBuffer(T* buffer, const uint8_t size )
      : m_size(size), m_buff(buffer)
    {
      clear();
    }

    /** Clear all entries in the circular buffer. */
    void clear(void)
    {
      m_front = 0;
      m_fill  = 0;
    }

    /** Test if the circular buffer is empty */
    inline bool empty(void) const
    {
      return !m_fill;
    }

    /** Return the number of records stored in the buffer */
    inline uint8_t available(void) const
    {
      return m_fill;
    }

    /** Test if the circular buffer is full */
    inline bool full(void) const
    {
      return m_fill == m_size;
    }
    
    /** Aquire record on front of the buffer, for writing.
     * After filling the record, it has to be pushed to actually
     * add it to the buffer.
     * @return Pointer to record, or NULL when buffer is full.
     */
    T* getFront(void) const
    {
      DISABLE_IRQ;
      T* f = NULL;
      if (!full())
        f = get(m_front);
      RESTORE_IRQ;
      return f;
    }
    
    /** Push record to front of the buffer
     * @param record   Record to push. If record was aquired previously (using getFront) its
     *                 data will not be copied as it is already present in the buffer.
     * @return True, when record was pushed successfully.
     */
    bool pushFront(T* record)
    {
      bool ok = false;
      DISABLE_IRQ;
      if (!full())
      {
        T* f = get(m_front);
        if (f != record)
          *f = *record;
        m_front = (m_front+1) % m_size;
        m_fill++;
        ok = true;
      }
      RESTORE_IRQ;
      return ok;
    }

    /** Aquire record on back of the buffer, for reading.
     * After reading the record, it has to be pop'ed to actually
     * remove it from the buffer.
     * @return Pointer to record, or NULL when buffer is empty.
     */
    T* getBack(void) const
    {
      T* b = NULL;
      DISABLE_IRQ;
      if (!empty())
        b = get(back());
      RESTORE_IRQ;
      return b;
    }

    /** Remove record from back of the buffer.
     * @return True, when record was pop'ed successfully.
     */
    bool popBack(void)
    {
      bool ok = false;
      DISABLE_IRQ;
      if (!empty())
      {
        m_fill--;
        ok = true;
      }
      RESTORE_IRQ;
      return ok;
    }
    
  protected:
    inline T * get(const uint8_t idx) const
    {
      return &(m_buff[idx]);
    }
    inline uint8_t back(void) const
    {
      return (m_front - m_fill + m_size) % m_size;
    }

    const uint8_t      m_size;     // Total number of records that can be stored in the buffer.
    T* const           m_buff;     // Ptr to buffer holding all records.
    volatile uint8_t   m_front;    // Index of front element (not pushed yet).
    volatile uint8_t   m_fill;     // Amount of records currently pushed.
};

#endif // CircularBuffer_h
