#pragma once
#include <unordered_map>
#include <cassert>
using namespace std;

template<typename KeyType, typename ValueType>
class LFUCache
{
public:

    LFUCache(size_t cap)
        : capacity_(cap)
        , head_(nullptr)
        , tail_(nullptr)
        , countHead_(nullptr)
        , countTail_(nullptr)
    {
        head_ = new LFUCacheNode, tail_ = new LFUCacheNode;
        head_->next = tail_;
        tail_->prev = head_;
        countHead_ = new CountListNode, countTail_ = new CountListNode;
        countHead_->next = countTail_;
        countTail_->prev = countHead_;
    }

    ~LFUCache()
    {
        release();
        delete head_;
        delete tail_;
        delete countHead_;
        delete countTail_;
    }

    LFUCache(const LFUCache<KeyType, ValueType>& lhs) = delete;
    void operator=(const LFUCache<KeyType, ValueType>& lhs) = delete;

    LFUCache(LFUCache<KeyType, ValueType>&& lhs)
    {
        using std::swap;
        decltype(*this) tmp(0);
        swap(tmp.capacity_, lhs.capacity_);
        swap(tmp.head_, lhs.head_);
        swap(tmp.tail_, lhs.tail_);
        swap(tmp.countHashTbl_, lhs.countHashTbl_);
        swap(tmp.keyHashTbl_, lhs.keyHashTbl_);
    }

    LFUCache<KeyType, ValueType>& operator=(LFUCache<KeyType, ValueType>&& lhs)
    {
        if (this == &lhs) return *this;
        swap(capacity_, lhs.capacity_);
        swap(head_, lhs.head_);
        swap(tail_, lhs.tail_);
        swap(countHashTbl_, lhs.countHashTbl_);
        swap(keyHashTbl_, lhs.keyHashTbl_);
        lhs.release();
    }

    ValueType get(KeyType key)
    {
        auto node = find(key);
        if (node == nullptr) {
            return ValueType(-1);
        }

        update(key, node->value);
        return node->value;
    }

    void put(KeyType key, ValueType value)
    {
        if (capacity_ == 0) return;
        if (find(key) != nullptr) {
            update(key, value);
        }
        else {
            insert(key, value);
        }
    }

private:

    struct LFUCacheNode
    {
        KeyType key;
        ValueType value;
        size_t count;
        LFUCacheNode* prev, * next;

        LFUCacheNode()
            : LFUCacheNode(KeyType(), ValueType())
        {
        }

        LFUCacheNode(KeyType k, ValueType v)
            : key(k)
            , value(v)
            , count(1)
            , prev(nullptr)
            , next(nullptr)
        {
        }
    };

    struct CountListNode
    {
        size_t count;
        CountListNode* prev, * next;
        LFUCacheNode* cache;

        CountListNode()
            : CountListNode(0)
        {
        }

        CountListNode(size_t c)
            : count(c)
            , prev(nullptr)
            , next(nullptr)
            , cache(nullptr)
        {
        }
    };

    void release()
    {
        // cache
        auto node = head_->next;
        while (node != tail_) {
            auto tmp = node->next;
            delete node;
            node = tmp;
        }
        head_->next = tail_;
        tail_->prev = head_;
        decltype(keyHashTbl_)().swap(keyHashTbl_);

        // count
        auto count = countHead_->next;
        while (count != countTail_) {
            auto tmp = count->next;
            delete count;
            count = tmp;
        }
        countHead_->next = countTail_;
        countTail_->prev = countHead_;
        decltype(countHashTbl_)().swap(countHashTbl_);
    }

    LFUCacheNode* find(KeyType key)
    {
        if (keyHashTbl_.find(key) == keyHashTbl_.end()) {
            return nullptr;
        }
        return keyHashTbl_[key];
    }

    void takeNodeFromLFU(LFUCacheNode* node)
    {
        auto countNode = countHashTbl_[node->count];
        assert(countNode != nullptr);

        // If `cache` point to `node`, we need to update `cache` point to the following node of `node` or null
        if (countNode->cache == node) {
            if (node->next == tail_ || node->next->count != node->count) countNode->cache = nullptr;
            else countNode->cache = node->next;
        }

        node->next->prev = node->prev;
        node->prev->next = node->next;
        node->next = node->prev = nullptr;
    }

    void putNodeToLFU(LFUCacheNode* node)
    {
        auto countNode = countHashTbl_[node->count];
        // Caller make sure that `countNode` not equal to null
        assert(countNode != nullptr);

        if (countNode->cache != nullptr) {
            node->next = countNode->cache;
        }
        else {
            // Find next `cache` that is not null
            auto c = countNode->next;
            while (c != countTail_ && c->cache == nullptr) {
                c = c->next;
            }
            node->next = c == countTail_ ? tail_ : c->cache;
        }
        node->prev = node->next->prev;
        node->next->prev = node;
        node->prev->next = node;
        countNode->cache = node;
    }

    void update(KeyType key, ValueType value);
    void insert(KeyType key, ValueType value);

private:
    size_t capacity_;
    LFUCacheNode* head_, * tail_;
    unordered_map<KeyType, LFUCacheNode*> keyHashTbl_;
    CountListNode* countHead_, * countTail_;
    unordered_map<size_t, CountListNode*> countHashTbl_;
};

template<typename KeyType, typename ValueType>
void LFUCache<KeyType, ValueType>::update(KeyType key, ValueType value)
{
    auto node = find(key);
    if (node == nullptr) {
        return;
    }

    takeNodeFromLFU(node);

    size_t count = node->count;
    ++node->count;
    node->value = value;

    auto countNode = countHashTbl_[count];
    assert(countNode != nullptr);

    // Make sure that `count` is not null
    if (countNode->prev->count != node->count) {
        auto cn = new CountListNode(node->count);
        cn->next = countNode;
        cn->prev = countNode->prev;
        cn->next->prev = cn;
        cn->prev->next = cn;
        countHashTbl_[cn->count] = cn;
    }

    putNodeToLFU(node);
}

template<typename KeyType, typename ValueType>
void LFUCache<KeyType, ValueType>::insert(KeyType key, ValueType value)
{
    auto node = find(key);
    if (node != nullptr) {
        return;
    }

    if (keyHashTbl_.size() >= capacity_) {
        auto node = tail_->prev;
        takeNodeFromLFU(node);
        keyHashTbl_.erase(node->key);
        delete node;
    }

    node = new LFUCacheNode(key, value);
    keyHashTbl_[key] = node;

    auto countNode = countHashTbl_[1];
    if (countNode == nullptr) {
        countNode = new CountListNode(1);
        countNode->prev = countTail_->prev;
        countNode->next = countTail_;
        countNode->prev->next = countNode;
        countNode->next->prev = countNode;
        countHashTbl_[1] = countNode;
    }

    putNodeToLFU(node);
}
