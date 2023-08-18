// Copyright 2023 Corey Kosak
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <utility>
#include <vector>
#include "kosak/coding/coding.h"

namespace kosak::coding {

namespace internal {
void assertNotEmpty(bool empty);
}

// I have my own priority queue because std::priority_queue annoys me in four ways:
// 1. Can't .clear() the queue, so you have to rebuild the data structure from scratch, which means
//    you will be allocating/deallocating the underlying vector more than you need to. (I'd like
//    the underlying vector to keep its allocated capacity over multiple reuses of the PQ).
// 2. Can't change the underlying comparer (which is safe to do on an empty queue) without
//    rebuilding the data structure
// 3. Can't examine the comparer owned by the pq so I can use it in other contexts.
// 4. Orders from most to least rather than my preferred least to most :-)
// 5. Can't update in place
// Anyway, it's an easy data structure to implement, so I did.
template<typename T, typename Less = std::less<T>>
class PriorityQueue {
public:
  explicit PriorityQueue(Less less = Less()) : less_(std::move(less)) {}

  T &top() { internal::assertNotEmpty(empty()); return data_.front(); }

  template<typename U>
  void push(U &&item) {
    data_.push_back(std::forward<U>(item));
    bubbleUp(data_.size() - 1);
  }
  void pop();

  void fixTop() {
    bubbleDown(0);
  }

  std::vector<T> releaseUnderlying();

  void clear() { data_.clear(); }
  bool empty() const { return data_.empty(); }

private:
  void bubbleUp(size_t index);
  void bubbleDown(size_t index);

  std::vector<T> data_;
  Less less_;
};

template<typename T, typename Less>
void PriorityQueue<T, Less>::pop() {
  internal::assertNotEmpty(empty());
  using std::swap;
  swap(data_.front(), data_.back());
  data_.pop_back();
  bubbleDown(0);
}

template<typename T, typename Less>
std::vector<T> PriorityQueue<T, Less>::releaseUnderlying() {
  std::vector<T> result;
  result.swap(data_);
  return result;
}

// Structure of the vector: if a node is at index N, its two children are at index 2N+1 and 2N+2.
// Likewise, its parents are at trunc((N-1)/2)
template<typename T, typename Less>
void PriorityQueue<T, Less>::bubbleUp(size_t index) {
  while (index > 0) {
    size_t parent = (index - 1) / 2;
    // If data_[index] >= parent, then we are done.
    if (!less_(data_[index], data_[parent])) {
      return;
    }
    using std::swap;
    swap(data_[index], data_[parent]);
    index = parent;
  }
}

template<typename T, typename Less>
void PriorityQueue<T, Less>::bubbleDown(size_t index) {
  while (true) {
    size_t leftChild = 2 * index + 1;
    size_t rightChild = leftChild + 1;
    if (leftChild >= data_.size()) {
      // 'index' must be a leaf.
      return;
    }
    size_t smallerChild = leftChild;
    if (rightChild < data_.size() && less_(data_[rightChild], data_[leftChild])) {
      // There is a right child, and it's smaller than the left child.
      smallerChild = rightChild;
    }
    // If data[index] <= smaller child, then we are done
    // === smaller child >= data[index]
    // === !(smaller_child < data[index])
    if (!less_(data_[smallerChild], data_[index])) {
      return;
    }
    using std::swap;
    swap(data_[index], data_[smallerChild]);
    index = smallerChild;
  }
}

}  // namespace kosak::coding
