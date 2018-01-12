// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_UTIL_ARENA_H_
#define STORAGE_LEVELDB_UTIL_ARENA_H_

#include <vector>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include "port/port.h"

namespace leveldb {
// 一个内存管理模块，用于申请大块和小块的内存，优化内存使用。
// 避免申请很多小块内存，形式内存碎片。
// 申请大块内存时直接申请，申请小块内存时先申请一个大块内存，然后在大块内存上分配小块内存。
class Arena {
 public:
  Arena();
  ~Arena();

  // Return a pointer to a newly allocated memory block of "bytes" bytes.
  // 申请内存，返回一个内存块指针，内存块大小为bytes
  char* Allocate(size_t bytes);

  // Allocate memory with the normal alignment guarantees provided by malloc
  //申请内存，返回一个对齐的内存块指针。
  // (例如在4字节对齐的系统上，返回的地址是4的倍数，这样能提高效率,
  // 例如在存储int数组时，int时4个字节，应该申请对齐的内存块)
  char* AllocateAligned(size_t bytes);

  // Returns an estimate of the total memory usage of data allocated
  // by the arena.
  // 返回使用了多少内存。
  size_t MemoryUsage() const {
    return reinterpret_cast<uintptr_t>(memory_usage_.NoBarrier_Load());
  }

 private:
  char* AllocateFallback(size_t bytes);
  char* AllocateNewBlock(size_t block_bytes);

  // Allocation state
  char* alloc_ptr_; // 当前正在使用的内存块，剩余的内存开始指针
  size_t alloc_bytes_remaining_; // 当前的大内存块还剩多少内存

  // Array of new[] allocated memory blocks
  // 申请的所有内存块。
  std::vector<char*> blocks_;

  // Total memory usage of the arena.
  // 使用内存的总量。
  port::AtomicPointer memory_usage_;

  // 私有，禁止拷贝构造和赋值
  // No copying allowed
  Arena(const Arena&);
  void operator=(const Arena&);
};

inline char* Arena::Allocate(size_t bytes) {
  // The semantics of what to return are a bit messy if we allow
  // 0-byte allocations, so we disallow them here (we don't need
  // them for our internal use).
  //内存大小不能为0；
  assert(bytes > 0);
  // 如果要申请的内存小于剩余数。不用申请，直径在现有的内存块上取。
  if (bytes <= alloc_bytes_remaining_) {
    char* result = alloc_ptr_;
    alloc_ptr_ += bytes;
    alloc_bytes_remaining_ -= bytes;
    return result;
  }
  // 申请的内存比现有剩余的内存多，直径申请
  return AllocateFallback(bytes);
}

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_UTIL_ARENA_H_
