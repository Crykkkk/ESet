// the implementation of Red Black Tree，base on "Introduction to Algorithms"

#ifndef SJTU_RBTREE_HPP
#define SJTU_RBTREE_HPP

#include "exception.hpp"
#include <utility> 
#include <cstddef>
#include <bits/stl_function.h>

namespace sjtu {

enum class Color { RED, BLACK };

template<class Key, class Compare = std::less<Key>>
class Rbtree {
private:
    // 节点结构体
    struct Node {
        Key* value;   
        Node* parent;
        Node* left;
        Node* right;
        Color color;
        size_t size;   // 记录以该节点为根的子树大小，用于 O(log n) 的 range
        
        Node()
            : value(nullptr), parent(this), left(this), right(this),
              color(Color::BLACK), size(0) {}

        explicit Node(Key* val, Node* nil)
            : value(val), parent(nil), left(nil), right(nil),
              color(Color::RED), size(1) {}

        ~Node() {
            delete value;
        }
    };

    // 树的根节点和哨兵节点
    Node* root;
    Node* nil;         // 哨兵节点，替代 nullptr，它的 color 永远是 BLACK，size 是 0

    Compare comp;      // 比较器实例

public:
    // 迭代器
    class iterator {
    private:
        Node* current;
        Node* nil_ptr; // 迭代器需要知道 nil 是谁，才能判断边界
        Node* const* root_ptr;

    public:
        iterator();
        // 迭代器构造函数
        iterator(Node* node, Node* nil, Node* const* rt);

        // 迭代器核心操作
        const Key& operator*() const;
        iterator& operator++();    // 前置 ++
        iterator operator++(int);  // 后置 ++
        iterator& operator--();    // 前置 --
        iterator operator--(int);  // 后置 --
        bool operator==(const iterator& rhs) const;
        bool operator!=(const iterator& rhs) const;
    };

private:

    // 旋转操作
    void left_rotate(Node* x);
    void right_rotate(Node* x);

    // 维护子树大小
    void maintain_size(Node* x);

    // 插入与删除的颜色修复
    void insert_fixup(Node* z);
    void erase_fixup(Node* x);

    // 节点替换
    void transplant(Node* u, Node* v);

    // 寻找子树的最小/最大节点 
    Node* tree_minimum(Node* x) const;
    Node* tree_maximum(Node* x) const;

    // 内存清理和深拷贝辅助
    void clear_helper(Node* x);
    Node* copy_helper(Node* x, Node* parent, Node* other_nil);

    // 排名计算辅助 
    size_t get_rank(const Key& key) const; 

public:

    // 生命周期
    Rbtree();
    ~Rbtree();
    Rbtree(const Rbtree& other);
    Rbtree& operator=(const Rbtree& other);
    Rbtree(Rbtree&& other) noexcept;
    Rbtree& operator=(Rbtree&& other) noexcept;

    template<class... Args>
    std::pair<iterator, bool> emplace(Args&&... args);

    size_t erase(const Key& key);
    iterator find(const Key& key) const;
    
    // 区间查询
    size_t range(const Key& l, const Key& r) const;
    
    // 边界查询
    iterator lower_bound(const Key& key) const;
    iterator upper_bound(const Key& key) const;
    
    // 状态查询
    size_t size() const noexcept;
    iterator begin() const noexcept;
    iterator end() const noexcept;
};

template<class Key, class Compare>
Rbtree<Key, Compare>::iterator::iterator()
    : current(nullptr), nil_ptr(nullptr), root_ptr(nullptr) {}

template<class Key, class Compare>
Rbtree<Key, Compare>::iterator::iterator(Node* node, Node* nil, Node* const* rt)
    : current(node), nil_ptr(nil), root_ptr(rt) {}

template<class Key, class Compare>
const Key& Rbtree<Key, Compare>::iterator::operator*() const {
    if (current == nullptr || current == nil_ptr) {
        throw invalid_iterator();
    }
    return *current->value;
}

template<class Key, class Compare>
typename Rbtree<Key, Compare>::iterator& Rbtree<Key, Compare>::iterator::operator++() {
    if (current == nullptr || current == nil_ptr) return *this;
    if (current->right != nil_ptr) {
        current = current->right;
        while (current->left != nil_ptr) current = current->left;
    } else {
        Node* y = current->parent;
        while (y != nil_ptr && current == y->right) {
            current = y;
            y = y->parent;
        }
        current = y;
    }
    return *this;
}

template<class Key, class Compare>
typename Rbtree<Key, Compare>::iterator Rbtree<Key, Compare>::iterator::operator++(int) {
    iterator tmp = *this;
    ++(*this);
    return tmp;
}

template<class Key, class Compare>
typename Rbtree<Key, Compare>::iterator& Rbtree<Key, Compare>::iterator::operator--() {
    if (current == nullptr) return *this;
    if (current == nil_ptr) {
        Node* x = root_ptr == nullptr ? nil_ptr : *root_ptr;
        if (x == nil_ptr) return *this;
        while (x->right != nil_ptr) x = x->right;
        current = x;
        return *this;
    }
    if (current->left != nil_ptr) {
        current = current->left;
        while (current->right != nil_ptr) current = current->right;
    } else {
        Node* x = current;
        Node* y = current->parent;
        while (y != nil_ptr && x == y->left) {
            x = y;
            y = y->parent;
        }
        if (y != nil_ptr) current = y;
    }
    return *this;
}

template<class Key, class Compare>
typename Rbtree<Key, Compare>::iterator Rbtree<Key, Compare>::iterator::operator--(int) {
    iterator tmp = *this;
    --(*this);
    return tmp;
}

template<class Key, class Compare>
bool Rbtree<Key, Compare>::iterator::operator==(const iterator& rhs) const {
    return current == rhs.current && nil_ptr == rhs.nil_ptr;
}

template<class Key, class Compare>
bool Rbtree<Key, Compare>::iterator::operator!=(const iterator& rhs) const {
    return !(*this == rhs);
}

template<class Key, class Compare>
Rbtree<Key, Compare>::Rbtree() : root(nullptr), nil(new Node()), comp(Compare()) {
    root = nil;
}

template<class Key, class Compare>
Rbtree<Key, Compare>::~Rbtree() {
    clear_helper(root);
    delete nil;
}

template<class Key, class Compare>
Rbtree<Key, Compare>::Rbtree(const Rbtree& other)
    : root(nullptr), nil(new Node()), comp(other.comp) {
    root = nil;
    root = copy_helper(other.root, nil, other.nil);
}

template<class Key, class Compare>
Rbtree<Key, Compare>& Rbtree<Key, Compare>::operator=(const Rbtree& other) {
    if (this == &other) return *this;
    Rbtree tmp(other);
    Node* old_root = root;
    Node* old_nil = nil;
    root = tmp.root;
    nil = tmp.nil;
    comp = tmp.comp;
    tmp.root = old_root;
    tmp.nil = old_nil;
    return *this;
}

template<class Key, class Compare>
Rbtree<Key, Compare>::Rbtree(Rbtree&& other) noexcept
    : root(other.root), nil(other.nil), comp(std::move(other.comp)) {
    other.nil = new Node();
    other.root = other.nil;
}

template<class Key, class Compare>
Rbtree<Key, Compare>& Rbtree<Key, Compare>::operator=(Rbtree&& other) noexcept {
    if (this == &other) return *this;
    clear_helper(root);
    delete nil;
    root = other.root;
    nil = other.nil;
    comp = std::move(other.comp);
    other.nil = new Node();
    other.root = other.nil;
    return *this;
}

template<class Key, class Compare>
void Rbtree<Key, Compare>::left_rotate(Node* x) {
    Node* y = x->right;
    x->right = y->left;
    if (y->left != nil) y->left->parent = x;
    y->parent = x->parent;
    if (x->parent == nil) root = y;
    else if (x == x->parent->left) x->parent->left = y;
    else x->parent->right = y;
    y->left = x;
    x->parent = y;
    y->size = x->size;
    x->size = x->left->size + x->right->size + 1;
}

template<class Key, class Compare>
void Rbtree<Key, Compare>::right_rotate(Node* x) {
    Node* y = x->left;
    x->left = y->right;
    if (y->right != nil) y->right->parent = x;
    y->parent = x->parent;
    if (x->parent == nil) root = y;
    else if (x == x->parent->right) x->parent->right = y;
    else x->parent->left = y;
    y->right = x;
    x->parent = y;
    y->size = x->size;
    x->size = x->left->size + x->right->size + 1;
}

template<class Key, class Compare>
void Rbtree<Key, Compare>::maintain_size(Node* x) {
    while (x != nil) {
        x->size = x->left->size + x->right->size + 1;
        x = x->parent;
    }
}

template<class Key, class Compare>
void Rbtree<Key, Compare>::insert_fixup(Node* z) {
    while (z->parent->color == Color::RED) {
        if (z->parent == z->parent->parent->left) {
            Node* y = z->parent->parent->right;
            if (y->color == Color::RED) {
                z->parent->color = Color::BLACK;
                y->color = Color::BLACK;
                z->parent->parent->color = Color::RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    left_rotate(z);
                }
                z->parent->color = Color::BLACK;
                z->parent->parent->color = Color::RED;
                right_rotate(z->parent->parent);
            }
        } else {
            Node* y = z->parent->parent->left;
            if (y->color == Color::RED) {
                z->parent->color = Color::BLACK;
                y->color = Color::BLACK;
                z->parent->parent->color = Color::RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    right_rotate(z);
                }
                z->parent->color = Color::BLACK;
                z->parent->parent->color = Color::RED;
                left_rotate(z->parent->parent);
            }
        }
    }
    root->color = Color::BLACK;
}

template<class Key, class Compare>
void Rbtree<Key, Compare>::erase_fixup(Node* x) {
    while (x != root && x->color == Color::BLACK) {
        if (x == x->parent->left) {
            Node* w = x->parent->right;
            if (w->color == Color::RED) {
                w->color = Color::BLACK;
                x->parent->color = Color::RED;
                left_rotate(x->parent);
                w = x->parent->right;
            }
            if (w->left->color == Color::BLACK && w->right->color == Color::BLACK) {
                w->color = Color::RED;
                x = x->parent;
            } else {
                if (w->right->color == Color::BLACK) {
                    w->left->color = Color::BLACK;
                    w->color = Color::RED;
                    right_rotate(w);
                    w = x->parent->right;
                }
                w->color = x->parent->color;
                x->parent->color = Color::BLACK;
                w->right->color = Color::BLACK;
                left_rotate(x->parent);
                x = root;
            }
        } else {
            Node* w = x->parent->left;
            if (w->color == Color::RED) {
                w->color = Color::BLACK;
                x->parent->color = Color::RED;
                right_rotate(x->parent);
                w = x->parent->left;
            }
            if (w->right->color == Color::BLACK && w->left->color == Color::BLACK) {
                w->color = Color::RED;
                x = x->parent;
            } else {
                if (w->left->color == Color::BLACK) {
                    w->right->color = Color::BLACK;
                    w->color = Color::RED;
                    left_rotate(w);
                    w = x->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = Color::BLACK;
                w->left->color = Color::BLACK;
                right_rotate(x->parent);
                x = root;
            }
        }
    }
    x->color = Color::BLACK;
}

template<class Key, class Compare>
void Rbtree<Key, Compare>::transplant(Node* u, Node* v) {
    if (u->parent == nil) root = v;
    else if (u == u->parent->left) u->parent->left = v;
    else u->parent->right = v;
    v->parent = u->parent;
}

template<class Key, class Compare>
typename Rbtree<Key, Compare>::Node* Rbtree<Key, Compare>::tree_minimum(Node* x) const {
    while (x != nil && x->left != nil) x = x->left;
    return x;
}

template<class Key, class Compare>
typename Rbtree<Key, Compare>::Node* Rbtree<Key, Compare>::tree_maximum(Node* x) const {
    while (x != nil && x->right != nil) x = x->right;
    return x;
}

template<class Key, class Compare>
void Rbtree<Key, Compare>::clear_helper(Node* x) {
    if (x == nil) return;
    clear_helper(x->left);
    clear_helper(x->right);
    delete x;
}

template<class Key, class Compare>
typename Rbtree<Key, Compare>::Node*
Rbtree<Key, Compare>::copy_helper(Node* x, Node* parent, Node* other_nil) {
    if (x == other_nil) return nil;
    Node* p = new Node(new Key(*x->value), nil);
    p->parent = parent;
    p->color = x->color;
    p->size = x->size;
    p->left = copy_helper(x->left, p, other_nil);
    p->right = copy_helper(x->right, p, other_nil);
    return p;
}

template<class Key, class Compare>
size_t Rbtree<Key, Compare>::get_rank(const Key& key) const {
    size_t ans = 0;
    Node* x = root;
    while (x != nil) {
        if (comp(*x->value, key)) {
            ans += x->left->size + 1;
            x = x->right;
        } else {
            x = x->left;
        }
    }
    return ans;
}

template<class Key, class Compare>
template<class... Args>
std::pair<typename Rbtree<Key, Compare>::iterator, bool>
Rbtree<Key, Compare>::emplace(Args&&... args) {
    Key* val = new Key(std::forward<Args>(args)...);
    Node* y = nil;
    Node* x = root;
    while (x != nil) {
        y = x;
        if (comp(*val, *x->value)) x = x->left;
        else if (comp(*x->value, *val)) x = x->right;
        else {
            delete val;
            return std::pair<iterator, bool>(iterator(x, nil, &root), false);
        }
    }
    Node* z = new Node(val, nil);
    z->parent = y;
    if (y == nil) root = z;
    else if (comp(*z->value, *y->value)) y->left = z;
    else y->right = z;
    maintain_size(y);
    insert_fixup(z);
    return std::pair<iterator, bool>(iterator(z, nil, &root), true);
}

template<class Key, class Compare>
size_t Rbtree<Key, Compare>::erase(const Key& key) {
    Node* z = root;
    while (z != nil) {
        if (comp(key, *z->value)) z = z->left;
        else if (comp(*z->value, key)) z = z->right;
        else break;
    }
    if (z == nil) return 0;

    Node* y = z;
    Node* x;
    Color y_color = y->color;
    if (z->left == nil) {
        x = z->right;
        Node* p = z->parent;
        transplant(z, z->right);
        maintain_size(p);
    } else if (z->right == nil) {
        x = z->left;
        Node* p = z->parent;
        transplant(z, z->left);
        maintain_size(p);
    } else {
        y = tree_minimum(z->right);
        y_color = y->color;
        x = y->right;
        if (y->parent == z) {
            x->parent = y;
        } else {
            Node* p = y->parent;
            transplant(y, y->right);
            maintain_size(p);
            y->right = z->right;
            y->right->parent = y;
        }
        transplant(z, y);
        y->left = z->left;
        y->left->parent = y;
        y->color = z->color;
        y->size = y->left->size + y->right->size + 1;
        maintain_size(y->parent);
    }
    delete z;
    if (y_color == Color::BLACK) erase_fixup(x);
    return 1;
}

template<class Key, class Compare>
typename Rbtree<Key, Compare>::iterator Rbtree<Key, Compare>::find(const Key& key) const {
    Node* x = root;
    while (x != nil) {
        if (comp(key, *x->value)) x = x->left;
        else if (comp(*x->value, key)) x = x->right;
        else return iterator(x, nil, &root);
    }
    return iterator(nil, nil, &root);
}

template<class Key, class Compare>
size_t Rbtree<Key, Compare>::range(const Key& l, const Key& r) const {
    if (comp(r, l)) return 0;
    size_t ans = get_rank(r) - get_rank(l);
    if (find(r) != end()) ++ans;
    return ans;
}

template<class Key, class Compare>
typename Rbtree<Key, Compare>::iterator Rbtree<Key, Compare>::lower_bound(const Key& key) const {
    Node* x = root;
    Node* ans = nil;
    while (x != nil) {
        if (!comp(*x->value, key)) {
            ans = x;
            x = x->left;
        } else {
            x = x->right;
        }
    }
    return iterator(ans, nil, &root);
}

template<class Key, class Compare>
typename Rbtree<Key, Compare>::iterator Rbtree<Key, Compare>::upper_bound(const Key& key) const {
    Node* x = root;
    Node* ans = nil;
    while (x != nil) {
        if (comp(key, *x->value)) {
            ans = x;
            x = x->left;
        } else {
            x = x->right;
        }
    }
    return iterator(ans, nil, &root);
}

template<class Key, class Compare>
size_t Rbtree<Key, Compare>::size() const noexcept {
    return root->size;
}

template<class Key, class Compare>
typename Rbtree<Key, Compare>::iterator Rbtree<Key, Compare>::begin() const noexcept {
    return iterator(tree_minimum(root), nil, &root);
}

template<class Key, class Compare>
typename Rbtree<Key, Compare>::iterator Rbtree<Key, Compare>::end() const noexcept {
    return iterator(nil, nil, &root);
}

} // namespace sjtu

#endif
