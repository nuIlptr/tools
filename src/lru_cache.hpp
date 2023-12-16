#pragma once
#include <unordered_map>
using std::unordered_map;

template<typename KeyType, typename ValueType>
class LRUCache {
 public:
  explicit LRUCache(size_t cap)
      : capacity_(cap), head_(new LRUCacheNode), tail_(new LRUCacheNode) {
    head_->next = tail_;
    tail_->prev = head_;
  }

  ~LRUCache() {
    release();
    delete head_;
    delete tail_;
  }

  LRUCache(const LRUCache<KeyType, ValueType> &lhs) = delete;
  void operator=(const LRUCache<KeyType, ValueType> &lhs) = delete;

  LRUCache(LRUCache<KeyType, ValueType> &&lhs) noexcept {
    capacity_ = lhs.capacity_;
    head_ = lhs.head_;
    tail_ = lhs.tail_;
    hash_ = std::move(lhs.hash_);

    // destructible
    lhs.capacity_ = 0;
    lhs.head_ = new LRUCacheNode;
    lhs.tail_ = new LRUCacheNode;
    lhs.head_->next = lhs.tail_;
    lhs.tail_->prev = lhs.head_;
  }

  LRUCache<KeyType, ValueType> &operator=(LRUCache<KeyType, ValueType> &&lhs) noexcept {
    if (&lhs != this) {
      using std::swap;
      swap(capacity_, lhs.capacity_);
      swap(head_, lhs.head_);
      swap(tail_, lhs.tail_);
      swap(hash_, lhs.hash_);

      // release
      lhs.release();
    }
    return *this;
  }

  ValueType get(KeyType key) {
    auto node = find(key);
    if (node == nullptr) {
      return -1;
    }
    takeNodeFromLRU(node);
    putNodeToLRU(node);
    return node->value;
  }

  void put(KeyType key, ValueType value) {
    if (capacity_ == 0) return;
    auto node = find(key);
    if (node == nullptr) {
      if (hash_.size() >= capacity_) {
        auto r = tail_->prev;
        takeNodeFromLRU(r);
        hash_.erase(r->key);
        delete r;
      }
      node = new LRUCacheNode(key, value);
      hash_[key] = node;
    } else {
      node->value = value;
      takeNodeFromLRU(node);
    }
    putNodeToLRU(node);
  }

 private:
  struct LRUCacheNode {
    KeyType key;
    ValueType value;
    LRUCacheNode *prev, *next;

    LRUCacheNode()
        : LRUCacheNode(KeyType(), ValueType()) {
    }

    LRUCacheNode(KeyType k, ValueType v)
        : key(k), value(v), prev(nullptr), next(nullptr) {
    }
  };

  LRUCacheNode *find(KeyType key) {
    if (hash_.find(key) == hash_.end()) {
      return nullptr;
    }
    return hash_[key];
  }

  void takeNodeFromLRU(LRUCacheNode *node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev = node->next = nullptr;
  }

  void putNodeToLRU(LRUCacheNode *node) {
    node->prev = head_;
    node->next = head_->next;
    node->prev->next = node;
    node->next->prev = node;
  }

  void release() {
    auto node = head_->next;
    while (node != tail_) {
      auto tmp = node->next;
      delete node;
      node = tmp;
    }

    head_->next = tail_;
    tail_->prev = head_;
    decltype(hash_)().swap(hash_);
    capacity_ = 0;
  }

 private:
  size_t capacity_;
  unordered_map<KeyType, LRUCacheNode *> hash_;
  LRUCacheNode *head_, *tail_;
};
