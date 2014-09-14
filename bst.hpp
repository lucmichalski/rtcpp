#include <vector>
#include <utility>
#include <iterator>

#include "bst_node.hpp"
#include "inorder_iterator.hpp"

template <typename T>
class bst {
  public:
  typedef T key_type;
  typedef node<T> node_type;
  typedef node_type* node_pointer;
  typedef inorder_iterator<T> const_iterator;
  typedef const_iterator iterator;
  private:
  typedef std::vector<node_type> pool_type;
  typedef typename pool_type::size_type size_type;
  std::vector<node_type> pool;
  node_type head;
  node_pointer avail;
  std::pair<iterator, bool> insert_node_right(node_pointer p, T key);
  std::pair<iterator, bool> insert_node_left(node_pointer p, T key);
  bst(const bst& rhs); // To be implemented
  node_pointer inorder_successor(node_pointer p) const;
  node_pointer inorder_predecessor(node_pointer p) const;
  const bst& operator=(const bst& rhs); // To be implemented
  public:
  bst(std::size_t reserve_n);
  std::pair<iterator, bool> insert(T key);
  const_iterator begin() const;
  const_iterator end() const {return const_iterator(&head);}
  const_iterator rbegin() const;
  const_iterator rend() const {return const_iterator(&head);}
};

template <typename T>
typename bst<T>::const_iterator bst<T>::begin() const
{
  typedef typename bst<T>::const_iterator const_iter;
  node_pointer q = head.llink;
  while (!(q->tag & 2))
    q = q->llink;

  return const_iter(q);
}

template <typename T>
typename bst<T>::const_iterator bst<T>::rbegin() const
{
  typedef typename bst<T>::const_iterator const_iter;
  node_pointer q = head.llink;
  while (!(q->tag & 1))
    q = q->rlink;

  return const_iter(const_iter(q));
}

template <typename T>
typename bst<T>::node_pointer bst<T>::inorder_successor(node_pointer p) const
{
  if (p->tag & 1)
    return p->rlink;

  node_pointer q = p->rlink;
  while (!(q->tag & 2))
    q = q->llink;

  return q;
}

template <typename T>
typename bst<T>::node_pointer bst<T>::inorder_predecessor(node_pointer p) const
{
  if (p->tag & 2)
    return p->llink;

  node_pointer q = p->llink;
  while (!(q->tag & 1))
    q = q->rlink;

  return q;
}

template <typename T>
bst<T>::bst(std::size_t reserve_n)
: pool(reserve_n == 0 ? 1 : reserve_n)
{
  head.llink = &head;
  head.rlink = &head;
  head.tag = 0;
  head.tag = head.tag | 2;

  const size_type pool_size = pool.size();
  // Let us link the avail stack.
  pool[0].llink = 0;
  pool[0].rlink = 0;
  for (std::size_t i = 1; i < pool_size; ++i) {
    pool[i].llink = &pool[i - 1];
    pool[i].rlink = 0;
  }
  avail = &pool.back();
}

template <typename T>
std::pair<typename bst<T>::iterator, bool> bst<T>::insert_node_right(node_pointer p, T key)
{
  typedef std::pair<typename bst<T>::iterator, bool> pair_type;
  if (!avail)
    return std::make_pair(&head, false); // The tree has exhausted its capacity.

  node_pointer q = avail;
  avail = avail->llink;
  q->key = key;
  q->rlink = p->rlink;
  q->tag = (q->tag & 2) | p->tag & 1;
  p->rlink = q;
  p->tag = p->tag & 2;
  q->llink = p;
  q->tag = q->tag | 2;

  if (!(q->tag & 1)) {
    node_pointer qs = inorder_successor(q);
    qs->llink = q;
  }

  return std::make_pair(q, true);
}

template <typename T>
std::pair<typename bst<T>::iterator, bool> bst<T>::insert_node_left(node_pointer p, T key)
{
  if (!avail)
    return std::make_pair(&head, false); // The tree has exhausted its capacity.

  node_pointer q = avail;
  avail = avail->llink;
  q->key = key;
  q->llink = p->llink;
  q->tag = (q->tag & 1) | p->tag & 2;
  p->llink = q;
  p->tag = p->tag & 1;
  q->rlink = p;
  q->tag = q->tag | 1;

  if (!(q->tag & 2)) {
    node_pointer qs = inorder_predecessor(q);
    qs->rlink = q;
  }

  return std::make_pair(q, true);
}

template <typename T>
std::pair<typename bst<T>::iterator, bool> bst<T>::insert(T key)
{
  if (head.tag & 2) // The tree is empty
    return insert_node_left(&head, key);

  node_pointer p = head.llink;
  for (;;) {
    if (key < p->key) {
      if (!(p->tag & 2)) {
        p = p->llink;
        continue;
      }
      return insert_node_left(p, key);
    } else if (key > p->key) {
      if (!(p->tag & 1)) {
        p = p->rlink;
        continue;
      }
      return insert_node_right(p, key);
    } else {
      return std::make_pair(p, false);
    }
  }
}

