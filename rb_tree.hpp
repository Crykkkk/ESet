// the implementation of Red Black Tree，base on "Introduction to Algorithms"

#ifndef SJTU_RBTREE_HPP
#define SJTU_RBTREE_HPP

#include "exception.hpp"
#include <utility> 
#include <cstddef>
#include <new>
#include <type_traits>
#include <bits/stl_function.h>

#ifndef ESET_INLINE_VALUE
#define ESET_INLINE_VALUE 0
#endif

#ifndef ESET_MEMORY_POOL
#define ESET_MEMORY_POOL 0
#endif

namespace sjtu {

enum class Color { RED, BLACK };

template<class Key, class Compare = std::less<Key>>
class Rbtree {
private:
    // 节点结构体
    struct Node {
#if ESET_INLINE_VALUE
        bool has_value;
        alignas(Key) unsigned char storage[sizeof(Key)];

        Key* value_ptr() {
            return reinterpret_cast<Key*>(storage);
        }

        const Key* value_ptr() const {
            return reinterpret_cast<const Key*>(storage);
        }
#else
        Key* value;   
#endif
        Node* parent;
        Node* left;
        Node* right;
        Color color;
        size_t size;   // 记录以该节点为根的子树大小，用于 O(log n) 的 range
        
        Node()
            :
#if ESET_INLINE_VALUE
              has_value(false),
#else
              value(nullptr),
#endif
              parent(this), left(this), right(this),
              color(Color::BLACK), size(0) {}

#if ESET_INLINE_VALUE
        template<class... Args>
        explicit Node(Node* nil, Args&&... args)
            : has_value(false), parent(nil), left(nil), right(nil),
              color(Color::RED), size(1) {
            new (storage) Key(std::forward<Args>(args)...);
            has_value = true;
        }

        ~Node() {
            if (has_value) value_ptr()->~Key();
        }
#else
        explicit Node(Key* val, Node* nil)
            : value(val), parent(nil), left(nil), right(nil),
              color(Color::RED), size(1) {}

        ~Node() = default;
#endif
    };

#if ESET_INLINE_VALUE && ESET_MEMORY_POOL
    enum { POOL_CHUNK_SIZE = 4096 };

    union PoolSlot {
        PoolSlot* next;
        typename std::aligned_storage<sizeof(Node), alignof(Node)>::type storage;
    };

    struct PoolChunk {
        PoolChunk* next;
        PoolSlot slots[POOL_CHUNK_SIZE];
    };
#endif

    // 树的根节点和哨兵节点
    Node* root;
    Node* nil;         // 哨兵节点，替代 nullptr，它的 color 永远是 BLACK，size 是 0
    Node* leftmost;
    Node* rightmost;

    Compare comp;      // 比较器实例

#if ESET_INLINE_VALUE && ESET_MEMORY_POOL
    PoolChunk* pool_chunks;
    PoolSlot* free_slots;
#endif

private:
#if ESET_INLINE_VALUE
    template<class... Args>
    Node* make_node(Node* nil, Args&&... args);
#else
    template<class... Args>
    static Key* make_value(Args&&... args);
    static void drop_value(Key* value) noexcept;
    Node* make_node(Key* value, Node* nil);
#endif
    static const Key& value_of(const Node* x);
    static Node* make_nil();
    void drop_node(Node* x) noexcept;
#if ESET_INLINE_VALUE && ESET_MEMORY_POOL
    void add_pool_chunk();
    void* pool_alloc();
    void pool_dealloc(Node* x) noexcept;
    void release_pool() noexcept;
#endif

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
    Node* successor(Node* x) const;
    Node* predecessor(Node* x) const;

    // 内存清理和深拷贝辅助
    void clear_helper(Node* x);
    Node* copy_helper(Node* x, Node* parent, Node* other_nil);

    // 排名计算辅助
    size_t rank_less(const Key& key) const;
    size_t rank_less_equal(const Key& key) const;

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
    return value_of(current);
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

#if ESET_INLINE_VALUE
template<class Key, class Compare>
template<class... Args>
typename Rbtree<Key, Compare>::Node*
Rbtree<Key, Compare>::make_node(Node* nil, Args&&... args) {
#if ESET_MEMORY_POOL
    void* mem = pool_alloc();
    try {
        return new (mem) Node(nil, std::forward<Args>(args)...);
    } catch (...) {
        pool_dealloc(reinterpret_cast<Node*>(mem));
        throw;
    }
#else
    return new Node(nil, std::forward<Args>(args)...);
#endif
}
#else
template<class Key, class Compare>
template<class... Args>
Key* Rbtree<Key, Compare>::make_value(Args&&... args) {
    return new Key(std::forward<Args>(args)...);
}

template<class Key, class Compare>
void Rbtree<Key, Compare>::drop_value(Key* value) noexcept {
    delete value;
}

template<class Key, class Compare>
typename Rbtree<Key, Compare>::Node*
Rbtree<Key, Compare>::make_node(Key* value, Node* nil) {
    return new Node(value, nil);
}
#endif

template<class Key, class Compare>
const Key& Rbtree<Key, Compare>::value_of(const Node* x) {
#if ESET_INLINE_VALUE
    return *x->value_ptr();
#else
    return *x->value;
#endif
}

template<class Key, class Compare>
typename Rbtree<Key, Compare>::Node* Rbtree<Key, Compare>::make_nil() {
    return new Node();
}

template<class Key, class Compare>
void Rbtree<Key, Compare>::drop_node(Node* x) noexcept {
    if (x == nullptr) return;
#if ESET_INLINE_VALUE && ESET_MEMORY_POOL
    if (!x->has_value) {
        delete x;
        return;
    }
    x->~Node();
    pool_dealloc(x);
#else
#if !ESET_INLINE_VALUE
    drop_value(x->value);
#endif
    delete x;
#endif
}

#if ESET_INLINE_VALUE && ESET_MEMORY_POOL
template<class Key, class Compare>
void Rbtree<Key, Compare>::add_pool_chunk() {
    PoolChunk* chunk = new PoolChunk();
    chunk->next = pool_chunks;
    pool_chunks = chunk;
    for (size_t i = 0; i < POOL_CHUNK_SIZE; ++i) {
        chunk->slots[i].next = free_slots;
        free_slots = &chunk->slots[i];
    }
}

template<class Key, class Compare>
void* Rbtree<Key, Compare>::pool_alloc() {
    if (free_slots == nullptr) add_pool_chunk();
    PoolSlot* slot = free_slots;
    free_slots = free_slots->next;
    return &slot->storage;
}

template<class Key, class Compare>
void Rbtree<Key, Compare>::pool_dealloc(Node* x) noexcept {
    PoolSlot* slot = reinterpret_cast<PoolSlot*>(x);
    slot->next = free_slots;
    free_slots = slot;
}

template<class Key, class Compare>
void Rbtree<Key, Compare>::release_pool() noexcept {
    PoolChunk* chunk = pool_chunks;
    while (chunk != nullptr) {
        PoolChunk* next = chunk->next;
        delete chunk;
        chunk = next;
    }
    pool_chunks = nullptr;
    free_slots = nullptr;
}
#endif

template<class Key, class Compare>
Rbtree<Key, Compare>::Rbtree()
    : root(nullptr), nil(make_nil()), leftmost(nullptr), rightmost(nullptr),
      comp(Compare())
#if ESET_INLINE_VALUE && ESET_MEMORY_POOL
      , pool_chunks(nullptr), free_slots(nullptr)
#endif
{
    root = nil;
    leftmost = nil;
    rightmost = nil;
}

template<class Key, class Compare>
Rbtree<Key, Compare>::~Rbtree() {
    clear_helper(root);
    drop_node(nil);
#if ESET_INLINE_VALUE && ESET_MEMORY_POOL
    release_pool();
#endif
}

template<class Key, class Compare>
Rbtree<Key, Compare>::Rbtree(const Rbtree& other)
    : root(nullptr), nil(make_nil()), leftmost(nullptr), rightmost(nullptr),
      comp(other.comp)
#if ESET_INLINE_VALUE && ESET_MEMORY_POOL
      , pool_chunks(nullptr), free_slots(nullptr)
#endif
{
    root = nil;
    root = copy_helper(other.root, nil, other.nil);
    leftmost = root == nil ? nil : tree_minimum(root);
    rightmost = root == nil ? nil : tree_maximum(root);
}

template<class Key, class Compare>
Rbtree<Key, Compare>& Rbtree<Key, Compare>::operator=(const Rbtree& other) {
    if (this == &other) return *this;
    Rbtree tmp(other);
    Node* old_root = root;
    Node* old_nil = nil;
    Node* old_leftmost = leftmost;
    Node* old_rightmost = rightmost;
#if ESET_INLINE_VALUE && ESET_MEMORY_POOL
    PoolChunk* old_pool_chunks = pool_chunks;
    PoolSlot* old_free_slots = free_slots;
#endif
    root = tmp.root;
    nil = tmp.nil;
    leftmost = tmp.leftmost;
    rightmost = tmp.rightmost;
    comp = tmp.comp;
#if ESET_INLINE_VALUE && ESET_MEMORY_POOL
    pool_chunks = tmp.pool_chunks;
    free_slots = tmp.free_slots;
#endif
    tmp.root = old_root;
    tmp.nil = old_nil;
    tmp.leftmost = old_leftmost;
    tmp.rightmost = old_rightmost;
#if ESET_INLINE_VALUE && ESET_MEMORY_POOL
    tmp.pool_chunks = old_pool_chunks;
    tmp.free_slots = old_free_slots;
#endif
    return *this;
}

template<class Key, class Compare>
Rbtree<Key, Compare>::Rbtree(Rbtree&& other) noexcept
    : root(other.root), nil(other.nil), leftmost(other.leftmost),
      rightmost(other.rightmost), comp(std::move(other.comp))
#if ESET_INLINE_VALUE && ESET_MEMORY_POOL
      , pool_chunks(other.pool_chunks), free_slots(other.free_slots)
#endif
{
#if ESET_INLINE_VALUE && ESET_MEMORY_POOL
    other.pool_chunks = nullptr;
    other.free_slots = nullptr;
#endif
    other.nil = make_nil();
    other.root = other.nil;
    other.leftmost = other.nil;
    other.rightmost = other.nil;
}

template<class Key, class Compare>
Rbtree<Key, Compare>& Rbtree<Key, Compare>::operator=(Rbtree&& other) noexcept {
    if (this == &other) return *this;
    clear_helper(root);
    drop_node(nil);
#if ESET_INLINE_VALUE && ESET_MEMORY_POOL
    release_pool();
#endif
    root = other.root;
    nil = other.nil;
    leftmost = other.leftmost;
    rightmost = other.rightmost;
    comp = std::move(other.comp);
#if ESET_INLINE_VALUE && ESET_MEMORY_POOL
    pool_chunks = other.pool_chunks;
    free_slots = other.free_slots;
    other.pool_chunks = nullptr;
    other.free_slots = nullptr;
#endif
    other.nil = make_nil();
    other.root = other.nil;
    other.leftmost = other.nil;
    other.rightmost = other.nil;
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
typename Rbtree<Key, Compare>::Node* Rbtree<Key, Compare>::successor(Node* x) const {
    if (x->right != nil) return tree_minimum(x->right);
    Node* y = x->parent;
    while (y != nil && x == y->right) {
        x = y;
        y = y->parent;
    }
    return y;
}

template<class Key, class Compare>
typename Rbtree<Key, Compare>::Node* Rbtree<Key, Compare>::predecessor(Node* x) const {
    if (x->left != nil) return tree_maximum(x->left);
    Node* y = x->parent;
    while (y != nil && x == y->left) {
        x = y;
        y = y->parent;
    }
    return y;
}

template<class Key, class Compare>
void Rbtree<Key, Compare>::clear_helper(Node* x) {
    if (x == nil) return;
    clear_helper(x->left);
    clear_helper(x->right);
    drop_node(x);
}

template<class Key, class Compare>
typename Rbtree<Key, Compare>::Node*
Rbtree<Key, Compare>::copy_helper(Node* x, Node* parent, Node* other_nil) {
    if (x == other_nil) return nil;
#if ESET_INLINE_VALUE
    Node* p = make_node(nil, value_of(x));
#else
    Node* p = make_node(make_value(value_of(x)), nil);
#endif
    p->parent = parent;
    p->color = x->color;
    p->size = x->size;
    p->left = copy_helper(x->left, p, other_nil);
    p->right = copy_helper(x->right, p, other_nil);
    return p;
}

template<class Key, class Compare>
size_t Rbtree<Key, Compare>::rank_less(const Key& key) const {
    size_t ans = 0;
    Node* x = root;
    while (x != nil) {
        if (comp(value_of(x), key)) {
            ans += x->left->size + 1;
            x = x->right;
        } else {
            x = x->left;
        }
    }
    return ans;
}

template<class Key, class Compare>
size_t Rbtree<Key, Compare>::rank_less_equal(const Key& key) const {
    size_t ans = 0;
    Node* x = root;
    while (x != nil) {
        if (!comp(key, value_of(x))) {
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
#if ESET_INLINE_VALUE
    Key val(std::forward<Args>(args)...);
    Node* y = nil;
    Node* x = root;
    while (x != nil) {
        y = x;
        if (comp(val, value_of(x))) x = x->left;
        else if (comp(value_of(x), val)) x = x->right;
        else return std::pair<iterator, bool>(iterator(x, nil, &root), false);
    }
    Node* z = make_node(nil, std::move(val));
#else
    Key* val = make_value(std::forward<Args>(args)...);
    Node* y = nil;
    Node* x = root;
    while (x != nil) {
        y = x;
        if (comp(*val, value_of(x))) x = x->left;
        else if (comp(value_of(x), *val)) x = x->right;
        else {
            drop_value(val);
            return std::pair<iterator, bool>(iterator(x, nil, &root), false);
        }
    }
    Node* z = make_node(val, nil);
#endif
    z->parent = y;
    if (y == nil) root = z;
    else if (comp(value_of(z), value_of(y))) y->left = z;
    else y->right = z;
    if (leftmost == nil || comp(value_of(z), value_of(leftmost))) leftmost = z;
    if (rightmost == nil || comp(value_of(rightmost), value_of(z))) rightmost = z;
    maintain_size(y);
    insert_fixup(z);
    return std::pair<iterator, bool>(iterator(z, nil, &root), true);
}

template<class Key, class Compare>
size_t Rbtree<Key, Compare>::erase(const Key& key) {
    Node* z = root;
    while (z != nil) {
        if (comp(key, value_of(z))) z = z->left;
        else if (comp(value_of(z), key)) z = z->right;
        else break;
    }
    if (z == nil) return 0;

    bool erase_leftmost = z == leftmost;
    bool erase_rightmost = z == rightmost;
    Node* next = erase_leftmost ? successor(z) : leftmost;
    Node* prev = erase_rightmost ? predecessor(z) : rightmost;

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
    drop_node(z);
    leftmost = next;
    rightmost = prev;
    if (y_color == Color::BLACK) erase_fixup(x);
    return 1;
}

template<class Key, class Compare>
typename Rbtree<Key, Compare>::iterator Rbtree<Key, Compare>::find(const Key& key) const {
    Node* x = root;
    while (x != nil) {
        if (comp(key, value_of(x))) x = x->left;
        else if (comp(value_of(x), key)) x = x->right;
        else return iterator(x, nil, &root);
    }
    return iterator(nil, nil, &root);
}

template<class Key, class Compare>
size_t Rbtree<Key, Compare>::range(const Key& l, const Key& r) const {
    if (comp(r, l)) return 0;
    return rank_less_equal(r) - rank_less(l);
}

template<class Key, class Compare>
typename Rbtree<Key, Compare>::iterator Rbtree<Key, Compare>::lower_bound(const Key& key) const {
    Node* x = root;
    Node* ans = nil;
    while (x != nil) {
        if (!comp(value_of(x), key)) {
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
        if (comp(key, value_of(x))) {
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
    // leftmost is maintained by insert/erase, so begin() is O(1).
    return iterator(leftmost, nil, &root);
}

template<class Key, class Compare>
typename Rbtree<Key, Compare>::iterator Rbtree<Key, Compare>::end() const noexcept {
    // end() is always the nil sentinel, so it is O(1).
    return iterator(nil, nil, &root);
}

} // namespace sjtu

#endif
