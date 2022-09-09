#pragma once
#include <unordered_map>
using namespace std;

template<typename KeyType, typename ValueType>
class LRUCache
{
public:
    LRUCache(size_t cap)
        : capacity_(cap)
        , head_(new LRUCacheNode)
        , tail_(new LRUCacheNode)
    {
        head_->next = tail_;
        tail_->prev = head_;
    }

    ValueType get(KeyType key)
    {
        auto node = find(key);
        if (node == nullptr) {
            return -1;
        }
        takeNodeFromLRU(node);
        putNodeToLRU(node);
        return node->value;
    }

    void put(KeyType key, ValueType value)
    {
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
        }
        else {
            node->value = value;
            takeNodeFromLRU(node);
        }
        putNodeToLRU(node);
    }

private:
    struct LRUCacheNode
    {
        KeyType key;
        ValueType value;
        LRUCacheNode* prev, * next;

        LRUCacheNode()
            : LRUCacheNode(KeyType(), ValueType())
        {
        }

        LRUCacheNode(KeyType k, ValueType v)
            : key(k)
            , value(v)
            , prev(nullptr)
            , next(nullptr)
        {
        }
    };

    LRUCacheNode* find(KeyType key)
    {
        if (hash_.find(key) == hash_.end()) {
            return nullptr;
        }
        return hash_[key];
    }

    void takeNodeFromLRU(LRUCacheNode* node)
    {
        node->prev->next = node->next;
        node->next->prev = node->prev;
        node->prev = node->next = nullptr;
    }

    void putNodeToLRU(LRUCacheNode* node)
    {
        node->prev = head_;
        node->next = head_->next;
        node->prev->next = node;
        node->next->prev = node;
    }

private:
    size_t capacity_;
    unordered_map<KeyType, LRUCacheNode*> hash_;
    LRUCacheNode* head_, * tail_;
};
