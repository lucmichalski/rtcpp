#include <utility>
#include <iterator>
#include <functional>
#include <algorithm>
#include <memory>

#include <utility/node_stack.hpp>
#include <utility/allocator.hpp>

#include "bst_iterator.hpp"

namespace rtcpp {

template < typename T
         , typename Compare = std::less<T>
         , typename Allocator = allocator<T>>
class bst { // Unbalanced binary search tree
  public:
  typedef bst_node<T> node_type;
  typedef T key_type;
  typedef T value_type;
  typedef std::size_t size_type;
  typedef Compare key_compare;
  typedef Compare value_compare;
  typedef typename std::allocator_traits<Allocator>::allocator_type allocator_type;
  typedef typename std::allocator_traits<Allocator>::template rebind_alloc<node_type> inner_allocator_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef typename std::allocator_traits<Allocator>::pointer pointer;
  typedef typename std::allocator_traits<Allocator>::const_pointer const_pointer;
  typedef std::ptrdiff_t difference_type;
  typedef bst_iterator<T> const_iterator;
  typedef const_iterator iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  private:
  typedef node_type* node_pointer;
  typedef const node_type* const_node_pointer;
  allocator_type m_alloc;
  inner_allocator_type m_inner_alloc;
  node_type head;
  Compare comp;
  void copy(bst& rhs) const noexcept;
  public:
  bst(const inner_allocator_type& alloc = allocator<node_type>()) noexcept;
  bst(const bst& rhs) noexcept;
  bst& operator=(const bst& rhs) noexcept;
  template <typename InputIt>
  bst(InputIt begin, InputIt end, const inner_allocator_type& alloc = allocator<node_type>()) noexcept;
  ~bst() noexcept;
  void clear() noexcept;
  std::pair<iterator, bool> insert(const value_type& key) noexcept;
  const_iterator begin() const noexcept {return const_iterator(inorder_successor(&head));}
  const_iterator end() const noexcept {return const_iterator(&head);}
  const_reverse_iterator rbegin() const noexcept {return const_reverse_iterator(end());}
  const_reverse_iterator rend() const noexcept {return const_reverse_iterator(begin());}
  key_compare key_comp() const noexcept {return comp;}
  value_compare value_comp() const noexcept {return comp;}
  size_type size() const noexcept {return std::distance(begin(), end());}
  bool empty() const noexcept {return begin() == end();}
  inner_allocator_type get_allocator() const noexcept {return m_inner_alloc;}
};

template <typename T, typename Compare, typename Allocator>
bst<T, Compare, Allocator>& bst<T, Compare, Allocator>::operator=(const bst<T, Compare, Allocator>& rhs) noexcept
{
  // This ctor can fail if the allocator runs out of memory.
  if (this != &rhs) {
    clear();
    rhs.copy(*this);
  }
  return *this;
}

template <typename T, typename Compare, typename Allocator>
bst<T, Compare, Allocator>::bst(const bst<T, Compare, Allocator>& rhs) noexcept
: m_inner_alloc(rhs.m_inner_alloc)
{
  // This ctor can fail if the allocator runs out of memory.
  head.llink = &head;
  head.rlink = &head;
  head.tag = detail::lbit;
  clear();
  rhs.copy(*this);
}

template <typename T, typename Compare, typename Allocator>
bst<T, Compare, Allocator>::bst(const inner_allocator_type& alloc) noexcept
: m_inner_alloc(alloc)
{
  head.llink = &head;
  head.rlink = &head;
  head.tag = detail::lbit;
}

template <typename T, typename Compare, typename Allocator>
template <typename InputIt>
bst<T, Compare, Allocator>::bst(InputIt begin, InputIt end, const inner_allocator_type& alloc) noexcept
: m_inner_alloc(alloc)
{
  head.llink = &head;
  head.rlink = &head;
  head.tag = detail::lbit;
  auto func = [this](const T& tmp) -> void {
    auto pair = this->insert(tmp);
    if (pair.second)
      return;
  };
  std::for_each(begin, end, func);
}

template <typename T, typename Compare, typename Allocator>
void bst<T, Compare, Allocator>::clear() noexcept
{
  node_pointer p = &head;
  for (;;) {
    node_pointer q = inorder_successor(p);
    if (p != &head) {
      std::allocator_traits<allocator_type>::destroy(m_alloc, &q->key);
      std::allocator_traits<inner_allocator_type>::deallocate(m_inner_alloc, p, 1);
    }
    if (q == &head)
      break;
    p = q;
  }
  head.llink = &head;
  head.rlink = &head;
  head.tag = detail::lbit;
}

template <typename T, typename Compare, typename Allocator>
bst<T, Compare, Allocator>::~bst() noexcept
{
  clear();
}

template <typename T, typename Compare, typename Allocator>
void bst<T, Compare, Allocator>::copy(bst<T, Compare, Allocator>& rhs) const noexcept
{
  const_node_pointer p = &head;
  node_pointer q = &rhs.head;

  for (;;) {
    if (!has_null_llink(p->tag)) {
      node_pointer tmp = std::allocator_traits<inner_allocator_type>::allocate(rhs.m_inner_alloc, 1);
      if (!tmp)
        break; // The tree has exhausted its capacity.

      attach_node_left(q, tmp);
    }

    p = preorder_successor(p);
    q = preorder_successor(q);

    if (p == &head)
      break;

    if (!has_null_rlink(p->tag)) {
      node_pointer tmp = std::allocator_traits<inner_allocator_type>::allocate(rhs.m_inner_alloc, 1);
      if (!tmp)
        break; // The tree has exhausted its capacity.

      attach_node_right(q, tmp);
    }

    q->key = p->key;
  }
}

template <typename T, typename Compare, typename Allocator>
std::pair<typename bst<T, Compare, Allocator>::iterator, bool>
bst<T, Compare, Allocator>::insert(const typename bst<T, Compare, Allocator>::value_type& key) noexcept
{
  typedef typename bst<T>::const_iterator const_iterator;
  if (has_null_llink(head.tag)) { // The tree is empty
    node_pointer q = std::allocator_traits<inner_allocator_type>::allocate(m_inner_alloc, 1);
    if (!q)
      return std::make_pair(const_iterator(), false); // The tree has exhausted its capacity.

    attach_node_left(&head, q);
    std::allocator_traits<allocator_type>::construct(m_alloc, std::addressof(q->key), key);
    return std::make_pair(const_iterator(q), true);
  }

  node_pointer p = head.llink;
  for (;;) {
    if (comp(key, p->key)) {
      if (!has_null_llink(p->tag)) {
        p = p->llink;
        continue;
      }
      node_pointer q = std::allocator_traits<inner_allocator_type>::allocate(m_inner_alloc, 1);
      if (!q)
        return std::make_pair(const_iterator(), false); // The tree has exhausted its capacity.

      attach_node_left(p, q);
      std::allocator_traits<allocator_type>::construct(m_alloc, std::addressof(q->key), key);
      return std::make_pair(q, true);
    } else if (comp(p->key, key)) {
      if (!has_null_rlink(p->tag)) {
        p = p->rlink;
        continue;
      }
      node_pointer q = std::allocator_traits<inner_allocator_type>::allocate(m_inner_alloc, 1);
      if (!q)
        return std::make_pair(const_iterator(), false); // The tree has exhausted its capacity.

      attach_node_right(p, q);
      std::allocator_traits<allocator_type>::construct(m_alloc, std::addressof(q->key), key);
      return std::make_pair(q, true);
    } else {
      return std::make_pair(p, false);
    }
  }
}

}

