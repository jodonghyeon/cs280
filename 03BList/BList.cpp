/*!
@file BList.cpp
@author Donghyeon Jo (donghyeon.jo)
@assignment BList
@course cs280 Spring
@date June 6, 2025
*//*______________________________________________________________________*/
#include "BList.h"


// -----------------------Public Methods--------------------------

/*!
@brief
  Basic Constructor of BList
  Creates empty BList
*//*______________________________________________________________*/
template <typename T, int Size>
BList<T, Size>::BList() :
  head_(nullptr), 
  tail_(nullptr), 
  stats_(sizeof(BNode), 0, Size, 0)
{ }

/*!
@brief
  Copy Constructor of BList
  Duplicates the given BList
@param
  rhs - A BList to copy
*//*______________________________________________________________*/
template <typename T, int Size>
BList<T, Size>::BList(const BList &rhs) :
  head_(nullptr), 
  tail_(nullptr), 
  stats_(rhs.stats_)
{
  BNode* curr = rhs.head_;
  while (curr)
  {
    BNode* new_node = nullptr;
    try{
      new_node = new BNode;
    }
    catch (std::bad_alloc& e){
      throw BListException(BListException::E_NO_MEMORY, e.what());
    }

    new_node->count = curr->count;
    for (int i = 0; i < curr->count; ++i){
      new_node->values[i] = curr->values[i];
    }
    new_node->next = nullptr;
    new_node->prev = nullptr;

    if (tail_ == nullptr){
      head_ = tail_ = new_node;
    }else{
      tail_->next = new_node;
      new_node->prev = tail_;
      tail_ = new_node;
    }

    curr = curr->next;
  }
}

/*!
@brief
  Destructor of BList
  Destroys the BList
*//*______________________________________________________________*/
template <typename T, int Size>
BList<T, Size>::~BList()
{
  BNode* curr = head_;
  while (curr)
  {
    BNode* next = curr->next;
    delete curr;
    curr = next;
  }
}

/*!
@brief
  Assignment operator of BList
  Copy the given BList
@param
  rhs - A BList to copy.
@return A Copied BList.
*//*______________________________________________________________*/
template <typename T, int Size>
BList<T, Size>& BList<T, Size>::operator=(const BList &rhs)
{
  if (this == &rhs){
    return *this;
  }

  clear();

  head_ = nullptr;
  tail_ = nullptr;
  stats_ = rhs.stats_;

  BNode* curr = rhs.head_;
  while (curr)
  {
    BNode* new_node = nullptr;
    try{
      new_node = new BNode;
    }
    catch (std::bad_alloc& e){
      throw BListException(BListException::E_NO_MEMORY, e.what());
    }

    new_node->count = curr->count;
    for (int i = 0; i < curr->count; ++i){
      new_node->values[i] = curr->values[i];
    }
    new_node->next = nullptr;
    new_node->prev = nullptr;

    if (tail_ == nullptr){
      head_ = tail_ = new_node;
    }else{
      tail_->next = new_node;
      new_node->prev = tail_;
      tail_ = new_node;
    }

    curr = curr->next;
  }

  return *this;
}

/*!
@brief
  Adds a new element with the given value as data at the end of the list (No sorting is performed).
@param
  value - Data to add.
*//*______________________________________________________________*/
template <typename T, int Size>
void BList<T, Size>::push_back(const T& value)
{
  if (tail_ == nullptr)
  {
    BNode* new_node = nullptr;
    try{
      new_node = new BNode;
    }
    catch (std::bad_alloc& e){
      throw BListException(BListException::E_NO_MEMORY, e.what());
    }
    new_node->next = nullptr;
    new_node->prev = nullptr;
    new_node->values[0] = value;
    new_node->count = 1;

    head_ = new_node;
    tail_ = new_node;

    stats_.NodeCount = 1;
    stats_.ItemCount = 1;
    return;
  }

  if (tail_->count < stats_.ArraySize)
  {
    tail_->values[tail_->count] = value;
    ++tail_->count;
    ++stats_.ItemCount;
    return;
  }

  BNode* new_node = nullptr;
  try{
    new_node = new BNode;
  }
  catch (std::bad_alloc& e){
    throw BListException(BListException::E_NO_MEMORY, e.what());
  }
  new_node->values[0] = value;
  new_node->count = 1;

  tail_->next = new_node;
  new_node->prev = tail_;
  tail_ = new_node;

  ++stats_.NodeCount;
  ++stats_.ItemCount;
}

/*!
@brief
  Adds a new element with the given value as data at the front of the list (No sorting is performed).
@param
  value - Data to add.
*//*______________________________________________________________*/
template <typename T, int Size>
void BList<T, Size>::push_front(const T& value)
{
  if (head_ == nullptr)
  {
    BNode* new_node = nullptr;
    try{
      new_node = new BNode;
    }
    catch (std::bad_alloc& e){
      throw BListException(BListException::E_NO_MEMORY, e.what());
    }
    new_node->next = nullptr;
    new_node->prev = nullptr;
    new_node->values[0] = value;
    new_node->count = 1;

    head_ = new_node;
    tail_ = new_node;

    stats_.NodeCount = 1;
    stats_.ItemCount = 1;
    return;
  }

  if (head_->count < stats_.ArraySize)
  {
    for (int i = head_->count - 1; i >= 0; --i){
      head_->values[i + 1] = head_->values[i];
    }
    head_->values[0] = value;
    ++head_->count;
    ++stats_.ItemCount;
    return;
  }

  BNode* new_node = nullptr;
  try{
    new_node = new BNode;
  }
  catch (std::bad_alloc& e){
    throw BListException(BListException::E_NO_MEMORY, e.what());
  }
  new_node->values[0] = value;
  new_node->count = 1;

  head_->prev = new_node;
  new_node->next = head_;
  head_ = new_node;

  ++stats_.NodeCount;
  ++stats_.ItemCount;
}

/*!
@brief
  Adds a new element with the given value as data at an appropriate position (Sorting is performed).
@param
  value - Data to add.
*//*______________________________________________________________*/
template <typename T, int Size>
void BList<T, Size>::insert(const T& value)
{
  if (tail_ == nullptr)
  {
    BNode* new_node = nullptr;
    try{
      new_node = new BNode;
    }
    catch (std::bad_alloc& e){
      throw BListException(BListException::E_NO_MEMORY, e.what());
    }
    new_node->next = nullptr;
    new_node->prev = nullptr;
    new_node->values[0] = value;
    new_node->count = 1;

    head_ = new_node;
    tail_ = new_node;

    stats_.NodeCount = 1;
    stats_.ItemCount = 1;
    return;
  }

  BNode* node_to_insert = nullptr;
  int offset_to_insert = 0;

  find_proper_location(value, node_to_insert, offset_to_insert);

  if (stats_.ArraySize == 1)
  {
    BNode* new_node = nullptr;
    try{
      new_node = new BNode;
    }
    catch (std::bad_alloc& e){
      throw BListException(BListException::E_NO_MEMORY, e.what());
    }

    new_node->values[0] = value;
    new_node->count = 1;

    if (offset_to_insert == 0)
    {
      new_node->prev = node_to_insert->prev;
      new_node->next = node_to_insert;
      if (node_to_insert->prev)
        node_to_insert->prev->next = new_node;
      else
        head_ = new_node;
      node_to_insert->prev = new_node;
    }
    else
    {
      new_node->next = node_to_insert->next;
      new_node->prev = node_to_insert;
      if (node_to_insert->next)
        node_to_insert->next->prev = new_node;
      else
        tail_ = new_node;
      node_to_insert->next = new_node;
    }

    ++stats_.NodeCount;
    ++stats_.ItemCount;
    return;
  }

  if (offset_to_insert == 0 && node_to_insert->prev && node_to_insert->prev->count < stats_.ArraySize)
  {
    node_to_insert->prev->values[node_to_insert->prev->count] = value;
    ++node_to_insert->prev->count;
  }
  else if (node_to_insert->count < stats_.ArraySize)
  {
    for (int i = node_to_insert->count; i > offset_to_insert; --i){
      node_to_insert->values[i] = node_to_insert->values[i - 1];
    }
    node_to_insert->values[offset_to_insert] = value;

    ++node_to_insert->count;
  }
  else
  {
    BNode* new_node = nullptr;
    try{
      new_node = new BNode;
    }
    catch (std::bad_alloc& e){
      throw BListException(BListException::E_NO_MEMORY, e.what());
    }

    int half = stats_.ArraySize / 2;
    for (int i = 0; i < half; ++i)
    {
      new_node->values[i] = node_to_insert->values[i + half];
      ++new_node->count;
      --node_to_insert->count;
    }

    if (offset_to_insert < half + 1)
    {
      for (int i = node_to_insert->count; i > offset_to_insert; --i){
        node_to_insert->values[i] = node_to_insert->values[i - 1];
      }
      node_to_insert->values[offset_to_insert] = value;
      ++node_to_insert->count;
    }
    else
    {
      int next_offset = offset_to_insert - half;
      for (int i = new_node->count; i > next_offset; --i){
        new_node->values[i] = new_node->values[i - 1];
      }
      new_node->values[next_offset] = value;
      ++new_node->count;
    }

    if (node_to_insert->next)
      node_to_insert->next->prev = new_node;
    
    new_node->next = node_to_insert->next;
    new_node->prev = node_to_insert;
    node_to_insert->next = new_node;

    if (node_to_insert == tail_)
    {
      tail_ = new_node;
    }
    ++stats_.NodeCount;
  }
  ++stats_.ItemCount;
}


/*!
@brief
  Removes the element at the given index.
@param
  index - index to remove.
*//*______________________________________________________________*/
template <typename T, int Size>
void BList<T, Size>::remove(int index)
{
  if (index < 0 || index >= stats_.ItemCount) {
    throw(BListException(BListException::E_BAD_INDEX, "remove: Index out of bounds"));
  }

  BNode* curr = head_;
  int counter = 0;
  while (curr) {
    if (counter + curr->count > index)
    {
      int offset = index - counter;

      for (int i = offset + 1; i < curr->count; ++i){
        curr->values[i - 1] = curr->values[i];
      }
      --curr->count;
      --stats_.ItemCount;

      if (curr->count == 0)
      {
        if (curr->prev){
          curr->prev->next = curr->next;
        }else{
          head_ = curr->next;
        }

        if (curr->next){
          curr->next->prev = curr->prev;
        }else{
          tail_ = curr->prev;
        }

        delete curr;
        --stats_.NodeCount;
      }

      return;
    }

    counter += curr->count;
    curr = curr->next;
  }

  throw(BListException(BListException::E_BAD_INDEX, "remove: Never"));
}

/*!
@brief
  Removes elements with a value equal to a given value.
@param
  value - value to remove.
*//*______________________________________________________________*/
template <typename T, int Size>
void BList<T, Size>::remove_by_value(const T& value)
{
  if (head_ == nullptr || stats_.ItemCount == 0){
      return;
  }

  int idx = find(value);
  while (idx != -1)
  {
    remove(idx);
    idx = find(value);
  }
}

/*!
@brief
  Find the element with a value equal to given value.
@param
  value - value to find.
@return An element's index.
*//*______________________________________________________________*/
template <typename T, int Size>
int BList<T, Size>::find(const T& value) const
{
  if (head_ == nullptr || stats_.ItemCount == 0){
    return -1;
  }

  const BNode* curr = head_;
  int counter = 0;

  while (curr){
    for (int i = 0; i < curr->count; ++i){
      if (curr->values[i] == value){
        return counter + i;
      }
    }
    counter += curr->count;
    curr = curr->next;
  }

  return -1;
}

/*!
@brief
  Lhs Subscript operator of BList
  Returns a reference to the data at the given index.
@param
  index - Index to get data from.
@return A reference to the data.
*//*______________________________________________________________*/
template <typename T, int Size>
T& BList<T, Size>::operator[](int index)
{
  if (index < 0 || index >= stats_.ItemCount){
    throw(BListException(BListException::E_BAD_INDEX, "operator[]: Index out of bounds"));
  }

  BNode* curr = head_;
  int counter = 0;

  while (curr)
  {
    if (counter + curr->count > index){
      return curr->values[index - counter];
    }

    counter += curr->count;
    curr = curr->next;
  }

  throw(BListException(BListException::E_BAD_INDEX, "operator[]: Never"));
}

/*!
@brief
  Rhs Subscript operator of BList
  Returns a reference to the data at the given index.
@param
  index - Index to get data from.
@return A reference to the data.
*//*______________________________________________________________*/
template <typename T, int Size>
const T& BList<T, Size>::operator[](int index) const
{
  if (index < 0 || index >= stats_.ItemCount){
    throw(BListException(BListException::E_BAD_INDEX, "operator[] const: Index out of bounds"));
  }

  BNode* curr = head_;
  int counter = 0;

  while (curr)
  {
    if (counter + curr->count > index){
      return curr->values[index - counter];
    }

    counter += curr->count;
    curr = curr->next;
  }

  throw(BListException(BListException::E_BAD_INDEX, "operator[] const: Never"));
}

/*!
@brief
  Returns Number of BList's total elements 
@return Number of tatal elements
*//*______________________________________________________________*/
template <typename T, int Size>
size_t BList<T, Size>::size() const
{
  return stats_.ItemCount;
}

/*!
@brief
  Clears the list.
*//*______________________________________________________________*/
template <typename T, int Size>
void BList<T, Size>::clear()
{
  BNode* curr = head_;
  while (curr)
  {
    BNode* next = curr->next;
    delete curr;
    curr = next;
  }

  head_ = nullptr;
  tail_ = nullptr;

  stats_.NodeCount = 0;
  stats_.ItemCount = 0;
}

/*!
@brief
  Returns single nodesize
@return size of BNode
*//*______________________________________________________________*/
template <typename T, int Size>
size_t BList<T, Size>::nodesize(void)
{
  return sizeof(BNode);
}

/*!
@brief
  Returns head of the list
@return head of the list
*//*______________________________________________________________*/
template <typename T, int Size>
const typename BList<T, Size>::BNode* BList<T, Size>::GetHead() const
{
  return head_;
}

/*!
@brief
  Returns stats of list
@return stats of list
*//*______________________________________________________________*/
template <typename T, int Size>
BListStats BList<T, Size>::GetStats() const
{
  return stats_;
}





// -----------------------Helper Methods--------------------------


/*!
@brief
  Find proper location for given value
@param
  value - value to find proper location.
  return_node - returning node location
  return_offset - returning offset location
*//*______________________________________________________________*/
template <typename T, int Size>
void BList<T, Size>::find_proper_location(const T& value, BNode*& return_node, int& return_offset)
{
  BNode* curr = head_;

  while (curr)
  {
    
    for (int i = 0; i < curr->count; ++i)
    {
      if (value < curr->values[i])
      {
        return_node = curr;
        return_offset = i;
        return;
      }
    }

    curr = curr->next;
  }

  return_node = tail_;
  return_offset = tail_ ? tail_->count : 0;
}