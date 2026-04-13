// the implementation of Red Black Tree，base on "Introduction to Algorithms"

#ifndef SJTU_RBTREE_HPP
#define SJTU_RBTREE_HPP

#include "exception.hpp"
#include <utility> 
#include <cstddef>

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
        
        // 节点生命周期函数
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
        Node* root_ptr;// 辅助边界检查，或者直接传 tree 的指针

    public:
        // 迭代器构造函数
        iterator(Node* node, Node* nil, Node* rt);

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

} // namespace sjtu

#endif