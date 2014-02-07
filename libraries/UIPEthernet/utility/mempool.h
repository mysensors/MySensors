/*
 mempool.h - sleek implementation of a memory pool
 Copyright (c) 2013 Norbert Truchsess <norbert.truchsess@t-online.de>
 All rights reserved.

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MEMPOOL_H
#define MEMPOOL_H

#include <inttypes.h>

#define POOLSTART 0
#define NOBLOCK 0

#include "mempool_conf.h"

//#ifdef MEMBLOCK_MV
//#define memblock_mv_cb(dest,src,size) MEMBLOCK_MV(dest,src,size)
//#endif

#ifdef MEMBLOCK_ALLOC
#define memblock_alloc_cb(address,size) MEMBLOCK_ALLOC(address,size)
#endif

#ifdef MEMBLOCK_FREE
#define memblock_free_cb(address,size) MEMBLOCK_FREE(address,size)
#endif

struct memblock
{
  memaddress begin;
  memaddress size;
  memhandle nextblock;
};

class MemoryPool
{
#ifdef MEMPOOLTEST_H
  friend class MemoryPoolTest;
#endif

protected:
  memaddress poolsize;
  struct memblock blocks[NUM_MEMBLOCKS+1];
#ifdef MEMBLOCK_MV
  virtual void memblock_mv_cb(memaddress dest, memaddress src, memaddress size) = 0;
#endif

public:
  MemoryPool(memaddress start, memaddress size);
  memhandle
  allocBlock(memaddress);
  void freeBlock(memhandle);
  void resizeBlock(memhandle handle, memaddress position);
  void resizeBlock(memhandle handle, memaddress position, memaddress size);
  memaddress blockSize(memhandle);
};
#endif
