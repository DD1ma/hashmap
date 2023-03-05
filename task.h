//
// Created by Zorin on 27.01.2023.
//

#ifndef UNTITLED_HASH_MAP_H
#define UNTITLED_HASH_MAP_H

#endif //UNTITLED_HASH_MAP_H

#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <vector>
#include <utility>

template <class KeyType, class ValueType, class Hash = std::hash<KeyType>,
        typename KeyEqual = std::equal_to<void>>
class HashMap {
public:
    using key_type = KeyType;
    using mapped_type = ValueType;
    using value_type = std::pair<const KeyType, ValueType>;
    using size_type = std::size_t;
    using key_equal = KeyEqual;

    struct Hash_node {
        value_type key_value;
        int64_t dist = -1;

        const bool operator==(const Hash_node &other) {
            return (std::tie(key_value, dist) == std::tie(other.key_value, other.dist));
        }
        bool operator!=(const Hash_node &other) {
            return (std::tie(key_value, dist) != std::tie(other.key_value, other.dist));
        }
        std::pair<KeyType &, ValueType &> ref() {
            return std::pair<KeyType &, ValueType &>(
                    const_cast<KeyType &>(key_value.first), key_value.second);
        }
        Hash_node &operator=(const Hash_node &other) {
            ref().first = other.key_value.first;
            ref().second = other.key_value.second;
            dist = other.dist;
            return *this;
        }
    };

    struct rhm_iterator {
    public:
        explicit rhm_iterator(const typename std::vector<Hash_node>::iterator &it) {
            idx_ = it;
        }
        rhm_iterator() : idx_(nullptr) {}

        bool operator==(const rhm_iterator &other) const {
            return other.idx_ == idx_;
        }
        bool operator!=(const rhm_iterator &other) const {
            return !(other == *this);
        }

        size_t operator-(const rhm_iterator &other) const {
            return idx_ - other.idx_;
        }

        rhm_iterator &operator++() {
            for (++idx_;; ++idx_) {
                if (idx_->dist != -1) {
                    return *this;
                }
            }
        }
        rhm_iterator operator++(int) {
            iterator temp = *this;
            for (++idx_;; ++idx_) {
                if (idx_->dist != -1) {
                    break;
                }
            }
            return temp;
        };

        value_type &operator*() const { return idx_->key_value; }
        value_type *operator->() const { return &idx_->key_value; }

    private:
        typename std::vector<Hash_node>::iterator idx_;

    };

    struct rhm_const_iterator {
    public:
        explicit rhm_const_iterator(const typename std::vector<Hash_node>::const_iterator &it) {
            idx_ = it;
        }

        rhm_const_iterator() : idx_(nullptr) {}

        bool operator==(const rhm_const_iterator &other) const {
            return other.idx_ == idx_;
        }

        bool operator!=(const rhm_const_iterator &other) const {
            return !(other == *this);
        }

        size_t operator-(const rhm_const_iterator &other) const {
            return idx_ - other.idx_;
        }

        rhm_const_iterator &operator++() {
            for (++idx_;; ++idx_) {
                if (idx_->dist != -1) {
                    return *this;
                }
            }
        }
        rhm_const_iterator operator++(int) {
            const_iterator temp = *this;
            for (++idx_;; ++idx_) {
                if (idx_->dist != -1) {
                    break;
                }
            }
            return temp;
        };

        const value_type &operator*() const { return idx_->key_value; }

        const value_type *operator->() const { return &idx_->key_value; }

    private:
        typename std::vector<Hash_node>::const_iterator idx_;

    };

    using iterator = rhm_iterator;
    using const_iterator = rhm_const_iterator;

public:
    HashMap(size_type bucket_count, Hash hash_func = Hash()) : hasher(hash_func)
    {
        size_t pow2 = 1;
        while (pow2 < bucket_count) {
            pow2 <<= 1;
        }
        size_ = 0;
        rhm_.resize(pow2 + 1);
        rhm_.back().dist = -2;
    }

    HashMap(Hash hash_func = Hash()) : hasher(hash_func) {
        size_ = 0;
        rhm_.resize(2);
        rhm_.back().dist = -2;
    }

    HashMap(const std::initializer_list<value_type> &all, Hash hash_func = Hash()) : HashMap(all.size(), hash_func) {
        for (const auto x: all) {
            insert(x);
        }
    }

    /*
     * HashMap(const HashMap &other, size_type bucket_count, Hash hash_func = Hash())
            : HashMap(bucket_count, hash_func) {
        for (auto it = other.begin(); it != other.end(); ++it) {
            insert(*it);
        }
    }
     */

    HashMap(iterator begin, iterator end, Hash hash_func = Hash()) : HashMap(end - begin, hash_func) {
        for (; begin != end; ++begin) {
            insert((*begin));
        }
    }

    // Iterators
    iterator begin() {
        //надеюсь можно за линию искать begin
        for (size_t ind = 0; ind < rhm_.size(); ++ind) {
            if (rhm_[ind].dist != -1) {
                return static_cast<iterator>(rhm_.begin() + ind);
            }
        }
        return static_cast<iterator>(rhm_.end() - 1);
    }

    const_iterator begin() const {
        //надеюсь можно за линию искать begin
        for (size_t ind = 0; ind < rhm_.size(); ++ind) {
            if (rhm_[ind].dist != -1) {
                return static_cast<const_iterator>(rhm_.begin() + ind);
            }
        }
        return static_cast<const_iterator>(rhm_.end() - 1);
    }

    iterator end() noexcept { return static_cast<iterator>(rhm_.end() - 1); }

    const_iterator end() const noexcept {
        return static_cast<const_iterator>(rhm_.end() - 1);
    }

    // Capacity
    bool empty() const noexcept { return size() == 0; }

    size_type size() const noexcept { return size_; }

    // Modifiers
    void clear() noexcept {
        rhm_.clear();
        rhm_.assign(2, Hash_node());
        rhm_.back().dist = -2;
        size_ = 0;
    }

    void insert(const value_type &value) {
        if(find(value.first) != end()) {
            return;
        }
        reserve(size_ + 1);
        size_++;
        emplace_impl(value.first, value.second);
    }

    template <typename... Args>
    void emplace(Args &&... args) {
        return emplace_impl(std::forward<Args>(args)...);
    }

    void erase(const KeyType &key) {
        auto it = find_impl(key);
        if (it != end()) {
            erase_impl(it);
        }
    }

    void swap(HashMap &other) noexcept {
        std::swap(rhm_, other.rhm_);
        std::swap(size_, other.size_);
    }

    // Lookup
    mapped_type &at(const key_type &key) { return at_impl(key); }

    template <typename K> mapped_type &at(const K &x) { return at_impl(x); }

    const mapped_type &at(const key_type &key) const { return at_impl(key); }

    template <typename K> const mapped_type &at(const K &x) const {
        return at_impl(x);
    }

    mapped_type& operator[](const key_type &key) {
        insert(value_type(key, ValueType()));
        return find(key)->second;
    }

    iterator find(const key_type &key) { return find_impl(key); }

    const_iterator find(const key_type &key) const { return find_impl(key); }

    // Hash policy
    void rehash(size_type count, bool flag) {
        std::vector<value_type> values;
        for (const auto &val : rhm_) {
            if (val.dist != -1 && val.dist != -2) {
                values.push_back(val.key_value);
            }
        }
        size_t sz = rhm_.size();
        clear();
        if(flag) {
            rhm_.assign(sz * 2 - 1, Hash_node());
        } else {
            rhm_.assign(sz / 2 + 1, Hash_node());
        }
        rhm_.back().dist = -2;
        for (const value_type &obj : values) {
            emplace_impl(obj.first, obj.second);
            size_++;
        }
    }

    void reserve(size_type count) {
        if (count * 2 > rhm_.size()) {
            rehash(count * 2, true);
        }
        if(count * 4 < rhm_.size() - 1) {
            rehash((count / 2), false);
        }
    }

    // Observers
    Hash hash_function() const { return hasher; }

    key_equal key_eq() const { return key_equal(); }

private:
    bool emplace_impl(KeyType key, ValueType value) {
        int64_t dist = 0;
        for (size_t idx = key_to_idx(key);; idx = probe_next(idx)) {
            if (rhm_[idx].dist == -1) {
                rhm_[idx].key_value.second = value;
                rhm_[idx].ref().first = key;
                rhm_[idx].dist = dist;
                return true;
            }
            if (rhm_[idx].key_value.first == key) {
                return false;
            }
            if (rhm_[idx].dist < dist) {
                Hash_node temp = rhm_[idx];
                rhm_[idx].ref().first = key;
                rhm_[idx].ref().second = value;
                rhm_[idx].dist = dist;
                key = temp.key_value.first;
                value = temp.key_value.second;
                dist = temp.dist;
            }
            dist++;
        }
    }

    void erase_impl(iterator it) {
        size_t bucket = it - static_cast<iterator>(rhm_.begin());
        rhm_[bucket].dist = -1;
        for (size_t idx = bucket;; idx = probe_next(idx)) {
            size_t nxt = probe_next(idx);
            if (rhm_[nxt].dist == -1 || rhm_[nxt].dist == 0) {
                break;
            }
            rhm_[idx].ref() = rhm_[nxt].ref();
            rhm_[idx].dist = rhm_[nxt].dist - 1;
            rhm_[nxt].dist = -1;
        }
        size_--;
        reserve(size_);
    }

    template <typename K> mapped_type &at_impl(const K &key) {
        iterator it = find_impl(key);
        if (it != end()) {
            return it->second;
        }
        throw std::out_of_range("HashMap::at");
    }

    template <typename K> const mapped_type &at_impl(const K &key) const {
        return const_cast<HashMap *>(this)->at_impl(key);
    }

     iterator find_impl(const KeyType &key) {
        int64_t dist = 0;
        for (size_t idx = key_to_idx(key);; idx = probe_next(idx)) {
            if (rhm_[idx].dist < dist) {
                break;
            }
            if (rhm_[idx].dist >= 0 && key == rhm_[idx].key_value.first) {
                return static_cast<iterator>(rhm_.begin() + idx);
            }
            dist++;
        }
        return end();
    }

    const_iterator find_impl(const KeyType &key) const {
        int64_t dist = 0;
        for (size_t idx = key_to_idx(key);; idx = probe_next(idx)) {
            if (rhm_[idx].dist < dist) {
                break;
            }
            if (rhm_[idx].dist >= 0 && key == rhm_[idx].key_value.first) {
                return static_cast<const_iterator>(rhm_.begin() + idx);
            }
            dist++;
        }
        return end();
    }


    template <typename K>
    size_t key_to_idx(const K &key) const noexcept {
        const size_t mask = rhm_.size() - 2;
        return hasher(key) & mask;
    }

    size_t probe_next(size_t idx) const noexcept {
        const size_t mask = rhm_.size() - 2;
        return (idx + 1) & mask;
    }

    size_t diff(size_t a, size_t b) const noexcept {
        const size_t mask = rhm_.size() - 2;
        return (rhm_.size() + (a - b)) & mask;
    }

private:
    std::vector<Hash_node> rhm_;
    size_t size_ = 0;
    Hash hasher;
};


