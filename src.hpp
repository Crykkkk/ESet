#ifndef ESET_HPP
#define ESET_HPP

#include "rb_tree.hpp"
#include "exception.hpp"
#include <functional>
#include <utility>

template<class Key, class Compare = std::less<Key>> 
class ESet {
public:
    using iterator = typename sjtu::Rbtree<Key, Compare>::iterator;

private:
    sjtu::Rbtree<Key, Compare> tree;

public:
    ESet() = default; // 直接使用 tree 的内容即可
    ~ESet() = default;
    
    ESet(const ESet& other) = default;
    ESet& operator=(const ESet& other) = default;
    
    ESet(ESet&& other) noexcept = default;
    ESet& operator=(ESet&& other) noexcept = default;
    
    template< class... Args >
    std::pair<iterator, bool> emplace( Args&&... args ) {
        return tree.emplace(std::forward<Args>(args)...);
    }

    size_t erase( const Key& key ) {
        return tree.erase(key);
    }

    iterator find( const Key& key ) const {
        return tree.find(key);
    }

    size_t range( const Key& l, const Key& r ) const {
        return tree.range(l, r);
    }

    size_t size() const noexcept {
        return tree.size();
    }

    iterator lower_bound( const Key& key ) const {
        return tree.lower_bound(key);
    }

    iterator upper_bound( const Key& key ) const {
        return tree.upper_bound(key);
    }

    iterator begin() const noexcept {
        return tree.begin();
    }

    iterator end() const noexcept {
        return tree.end();
    }
};

#endif