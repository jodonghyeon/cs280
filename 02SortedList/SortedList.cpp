/*!
@file SortedList.cpp
@author Donghyeon Jo (donghyeon.jo)
@assignment SortedList
@course cs280 Spring
@date May 31, 2025
*//*______________________________________________________________________*/
#include "SortedList.h"

//******************************************************************************
//******************************************************************************
// static
template <typename T, typename Pred>
unsigned SortedList<T, Pred>::nodesize(void)
{
  return sizeof(Node);
}

template<typename T, typename Pred>
bool SortedList<T, Pred>::ImplementedExtraCredit(void)
{
  return false;
}


//******************************************************************************
//******************************************************************************


// -----------------------Public Methods--------------------------


/*!
@brief
  Constructor of SortedList
  Creates SortedList
@param
  Sorter - A function to set this list's sorter that takes two T type variables as arguments and returns a bool value.
  Allocator - ObjectAllocator to use.
  SharedAllocator - Whether to share the allocator when copying a list.
*//*______________________________________________________________*/
template <typename T, typename Pred>
SortedList<T, Pred>::SortedList(Pred sorter, ObjectAllocator *Allocator, bool SharedAllocator) :
  head_(nullptr), tail_(nullptr), size_(0), objAllocator_(Allocator), shareAllocator_(SharedAllocator), sorter_(sorter)
{
  if (objAllocator_)
  {
    freeAllocator_ = false;
  }
  else
  {
    try{
      objAllocator_ = new ObjectAllocator(sizeof(Node), OAConfig(true));
    }
    catch (std::bad_alloc&){
      throw SortedListException(SortedListException::E_NO_MEMORY, "SortedList(): No system memory available.");
    }

    freeAllocator_ = true;
  }
}

/*!
@brief
  Copy Constructor of SortedList
  Duplicates the given SortedList
@param
  rhs - A SortedList to copy
*//*______________________________________________________________*/
template <typename T, typename Pred>
SortedList<T, Pred>::SortedList(const SortedList &rhs) :
  head_(nullptr), tail_(nullptr), size_(0), sorter_(rhs.sorter_)
{
  if (rhs.shareAllocator_)
  {
    objAllocator_ = rhs.objAllocator_;

    freeAllocator_ = false;
    shareAllocator_ = true;
  }
  else
  {
    try{
      objAllocator_ = new ObjectAllocator(sizeof(Node), OAConfig(true));
    }
    catch (std::bad_alloc&){
      throw SortedListException(SortedListException::E_NO_MEMORY, "SortedList(): No system memory available.");
    }

    freeAllocator_ = true;
    shareAllocator_ = false;
  }

  const Node *temp_rhs_node = rhs.head_;
  while (temp_rhs_node)
  {
    push_back(temp_rhs_node->Data);
    temp_rhs_node = temp_rhs_node->Next;
  }
}

/*!
@brief
  Destructor of SortedList
  Destroys the SortedList
*//*______________________________________________________________*/
template <typename T, typename Pred>
SortedList<T, Pred>::~SortedList()
{
  Node *previous = nullptr;
  Node *current = head_;
  while (current)
  {
    previous = current;
    current = current->Next;
    previous->Data.~T();
    objAllocator_->Free(previous);
  }

  if (freeAllocator_)
    delete objAllocator_;
}

/*!
@brief
  Assignment operator of SortedList
  Copy the given SortedList
@param
  rhs - A SortedList to copy.
@return A Copied SortedList.
*//*______________________________________________________________*/
template <typename T, typename Pred>
SortedList<T, Pred>& SortedList<T, Pred>::operator=(const SortedList &rhs)
{
  if (this != &rhs)
  {
    Node *previous = nullptr;
    Node *current = head_;
    while (current)
    {
      previous = current;
      current = current->Next;
      previous->Data.~T();
      objAllocator_->Free(previous);
    }

    if (freeAllocator_)
      delete objAllocator_;


    head_ = nullptr;
    tail_ = nullptr;
    size_ = 0;
    sorter_ = rhs.sorter_;

    if (rhs.shareAllocator_)
    {
      objAllocator_ = rhs.objAllocator_;

      freeAllocator_ = false;
      shareAllocator_ = true;
    }
    else
    {
      try{
        objAllocator_ = new ObjectAllocator(sizeof(Node), OAConfig(true));
      }
      catch (std::bad_alloc&){
        throw SortedListException(SortedListException::E_NO_MEMORY, "operator=: No system memory available.");
      }

      freeAllocator_ = true;
      shareAllocator_ = false;
    }

    const Node *temp_rhs_node = rhs.head_;
    while (temp_rhs_node)
      {
        push_back(temp_rhs_node->Data);
        temp_rhs_node = temp_rhs_node->Next;
      }
    }


  return *this;
}

/*!
@brief
  Subscript operator of SortedList
  Returns a reference to the data of the node at the given index.
@param
  index - Index to get data from.
@return A reference to the data.
*//*______________________________________________________________*/
template <typename T, typename Pred>
const T& SortedList<T, Pred>::operator[](size_t index) const
{
  if (size_ == 0 || index >= size_){
    throw SortedListException(SortedListException::E_CLIENT_ERROR, "operator[]: Index out of bounds");
  }

  const size_t half_size = static_cast<size_t>(size_ / 2);

  if (index <= half_size)
  {
    Node *temp_node = head_;

    for (size_t i = 0; i < index; i++)
    {
      temp_node = temp_node->Next;
    }
    return temp_node->Data;
  }
  else
  {
    Node *temp_node = tail_;

    for (size_t i = static_cast<size_t>(size_ - 1); i > index; i--)
    {
      temp_node = temp_node->Prev;
    }
    return temp_node->Data;
  }
}

/*!
@brief
  Adds a new node with the given value as data at the end of the list (No sorting is performed).
@param
  value - Data to add.
*//*______________________________________________________________*/
template <typename T, typename Pred>
void SortedList<T, Pred>::push_back(const T& value)
{
  if (!head_)
  {
    try{
      Node *mem = reinterpret_cast<Node*>(objAllocator_->Allocate());
      head_ = new (mem) Node(value);
    }
    catch (const OAException &e){
      throw SortedListException(SortedListException::E_NO_MEMORY, e.what());
    }

    head_->Prev = nullptr;
    head_->Next = nullptr;
    tail_ = head_;
  }
  else
  {
    try{
      Node *mem = reinterpret_cast<Node*>(objAllocator_->Allocate());
      tail_->Next = new (mem) Node(value);
    }
    catch (const OAException &e){
      throw SortedListException(SortedListException::E_NO_MEMORY, e.what());
    }

    tail_->Next->Prev = tail_;
    tail_->Next->Next = nullptr;

    tail_ = tail_->Next;
  }

  ++size_;
}

/*!
@brief
  Adds a new node with the given value as data at an appropriate position for the sorter of the list
@param
  value - Data to add.
*//*______________________________________________________________*/
template <typename T, typename Pred>
void SortedList<T, Pred>::insert(const T& value)
{
  if (!head_)
  {
    push_back(value);
    return;
  }

  Node *previous = nullptr;
  Node *current = head_;

  while (current && sorter_(current->Data, value))
  {
    previous = current;
    current = current->Next;
  }

  if (!previous)
  {
    try{
      Node *mem = reinterpret_cast<Node*>(objAllocator_->Allocate());
      previous = new (mem) Node(value);
    }
    catch (const OAException &e){
      throw SortedListException(SortedListException::E_NO_MEMORY, e.what());
    }

    previous->Prev = nullptr;
    previous->Next = current;
    current->Prev = previous;
    head_ = previous;

    ++size_;
  }
  else if (!current)
  {
    push_back(value);
  }
  else
  {
    Node *temp_node;
    try{
      Node *mem = reinterpret_cast<Node*>(objAllocator_->Allocate());
      temp_node = new (mem) Node(value);
    }
    catch (const OAException &e){
      throw SortedListException(SortedListException::E_NO_MEMORY, e.what());
    }

    temp_node->Prev = previous;
    temp_node->Next = current;
    previous->Next = temp_node;
    current->Prev = temp_node;

    ++size_;
  }
}

/*!
@brief
  Returns size of the list.
@return size of the list.
*//*______________________________________________________________*/
template <typename T, typename Pred>
unsigned SortedList<T, Pred>::size(void) const
{
  return size_;
}

/*!
@brief
  Clears the list.
*//*______________________________________________________________*/
template <typename T, typename Pred>
void SortedList<T, Pred>::clear(void)
{
  Node *temp_node = head_;

  while (temp_node)
  {
    Free(&(temp_node->Data));
    temp_node = temp_node->Next;
  }
  size_ = 0;
}

/*!
@brief
  Performs a 'selection sort' that matches the given sorter.
@param
  fn - The sort function to be used in this sort that takes two T type variables as arguments and returns a bool value.
*//*______________________________________________________________*/
template <typename T, typename Pred>
template <typename Sorter>
void SortedList<T, Pred>::selection_sort(Sorter fn)
{
  Node *temp_node1 = head_;
  Node *temp_node2 = nullptr;
  Node *temp_node3 = nullptr;

  for (unsigned i = 0; i < size_; i++)
  {
    temp_node2 = temp_node1;
    temp_node3 = temp_node1->Next;
    for (unsigned j = i + 1; j < size_; j++)
    {
      if (fn(temp_node3->Data, temp_node2->Data))
      {
        temp_node2 = temp_node3;
      }
      temp_node3 = temp_node3->Next;
    }
    swap(temp_node1, temp_node2);
    temp_node1 = temp_node2->Next;
  }
}

/*!
@brief
  Performs a 'merge sort' that matches the given sorter.
@param
  fn - The sort function to be used in this sort that takes two T type variables as arguments and returns a bool value.
*//*______________________________________________________________*/
template <typename T, typename Pred>
template <typename Sorter>
void SortedList<T, Pred>::merge_sort(Sorter fn)
{
  if (size_ < 2)
    return;

  unsigned group_size = 1;
  while (group_size < size_){
    Node* curr = head_;
    Node* new_head = nullptr;
    Node* new_tail = nullptr;

    while (curr){
      Node* left = curr;
      Node* l_end = left;
      unsigned left_count = 1;
      while (left_count < group_size && l_end->Next){
        l_end = l_end->Next;
        ++left_count;
      }

      Node* right = l_end->Next;

      if (right)
        right->Prev = nullptr;

      l_end->Next = nullptr;

      Node* r_end = right;
      unsigned right_count = 1;
      while (right && right_count < group_size && r_end->Next){
        r_end = r_end->Next;
        ++right_count;
      }

      Node* remainder = nullptr;
      if (r_end){
        remainder = r_end->Next;

        if (remainder)
          remainder->Prev = nullptr;

        r_end->Next = nullptr;
      }

      Node* l_ptr = left;
      Node* r_ptr = right;
      while (l_ptr && r_ptr){
        Node* chosen = (fn(l_ptr->Data, r_ptr->Data) ? l_ptr : r_ptr);
        
        if (chosen == l_ptr)
          l_ptr = l_ptr->Next;
        else r_ptr = r_ptr->Next;

        if (!new_head){
          new_head = chosen;
          new_tail = chosen;
          new_tail->Prev = nullptr;
          new_tail->Next = nullptr;
        }else{
          new_tail->Next = chosen;
          chosen->Prev = new_tail;
          new_tail = chosen;
          new_tail->Next = nullptr;
        }
      }

      Node* rem = l_ptr ? l_ptr : r_ptr;
      while (rem){
        Node* temp_next = rem->Next;
        if (!new_head){
          new_head = rem;
          new_tail = rem;
          new_tail->Prev = nullptr;
          new_tail->Next = nullptr;
        }else{
          new_tail->Next = rem;
          rem->Prev = new_tail;
          new_tail = rem;
          new_tail->Next = nullptr;
        }
        rem = temp_next;
      }

      curr = remainder;
    }

    head_ = new_head;
    tail_ = new_tail;
    group_size *= 2;
  }
}



// -----------------------Helper Methods--------------------------


/*!
@brief
  Swaps two nodes in the list.
@param
  a - The first node to be swapped.
  b - The second node to be swapped.
*//*______________________________________________________________*/
template <typename T, typename Pred>
void SortedList<T, Pred>::swap(Node *a, Node *b)
{
  if (a == b)
    return;

  Node* aPrev = a->Prev;
  Node* aNext = a->Next;
  Node* bPrev = b->Prev;
  Node* bNext = b->Next;

  if (aNext == b){
    if (aPrev)
      aPrev->Next = b;

    b->Prev = aPrev;
    b->Next = a;
    a->Prev = b;
    a->Next = bNext;

    if (bNext)
      bNext->Prev = a;
  }
  else if (bNext == a){
    if (bPrev)
      bPrev->Next = a;

    a->Prev = bPrev;
    a->Next = b;
    b->Prev = a;
    b->Next = aNext;

    if (aNext)
      aNext->Prev = b;
  }
  else{
    if (aPrev)
      aPrev->Next = b;
    if (bPrev)
      bPrev->Next = a;
    if (aNext)
      aNext->Prev = b;
    if (bNext)
      bNext->Prev = a;

    b->Prev = aPrev;
    b->Next = aNext;
    a->Prev = bPrev;
    a->Next = bNext;
  }

  if (head_ == a)
    head_ = b;
  else if (head_ == b)
    head_ = a;
  if (tail_ == a)
    tail_ = b;
  else if (tail_ == b)
    tail_ = a;
}