#include "BList.h"

template <typename T, int Size>
BList<T, Size>::BList() :
  head_(nullptr), 
  tail_(nullptr), 
  stats_(sizeof(BNode), 0, Size, 0)
{ }

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

template <typename T, int Size>
void BList<T, Size>::insert(const T& value)
{
  if (head_ == nullptr)
  {
    BNode* new_node = create_node(value);
    head_ = new_node;
    tail_ = new_node;
    return;
  }

  BNode* target = nullptr;
  int offset = 0;
  BNode* curr = head_;
  while (curr)
  {
    for (int i = 0; i < curr->count; ++i)
    {
      if (value < curr->values[i])
      {
        target = curr;
        offset = i;
        break;
      }
    }

    if (target)
      break;

    curr = curr->next;
  }

  if (!target) {
    push_back(value);
    return;
  }

  if (target->count < stats_.ArraySize)
  {
    insert_into_node(target, offset, value);
    ++stats_.ItemCount;
    return;
  }

  BNode* left = target->prev;
  BNode* right = target;
  if (left && right &&
      left->count == stats_.ArraySize && right->count == stats_.ArraySize &&
      left->values[left->count - 1] < value && value < right->values[0])
  {
    BNode* new_left = split_node(left);
    if (value < new_left->values[0]) {
      insert_into_node(left, left->count, value);
    } else {
      insert_into_node(new_left, 0, value);
    }
    ++stats_.ItemCount;
    return;
  }

  int split_index = target->count / 2;
  BNode* new_node = split_node(target);
  if (offset < split_index) {
    insert_into_node(target, offset, value);
  } else {
    int offset2 = offset - split_index;
    insert_into_node(new_node, offset2, value);
  }
  ++stats_.ItemCount;
}

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

template <typename T, int Size>
size_t BList<T, Size>::size() const
{
  return stats_.ItemCount;
}

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

template <typename T, int Size>
size_t BList<T, Size>::nodesize(void)
{
  return sizeof(BNode);
}

template <typename T, int Size>
const typename BList<T, Size>::BNode* BList<T, Size>::GetHead() const
{
  return head_;
}

template <typename T, int Size>
BListStats BList<T, Size>::GetStats() const
{
  return stats_;
}














template <typename T, int Size>
void BList<T, Size>::insert_into_node(BNode* node, int offset, const T& value)
{
  for (int i = node->count - 1; i >= offset; --i){
    node->values[i + 1] = node->values[i];
  }
  node->values[offset] = value;
  ++node->count;
}

template <typename T, int Size>
typename BList<T,Size>::BNode* BList<T, Size>::create_node(const T& value)
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
  ++stats_.NodeCount;
  ++stats_.ItemCount;

  return new_node;
}

template <typename T, int Size>
typename BList<T,Size>::BNode* BList<T, Size>::split_node(BNode* node)
{
  BNode* new_node = nullptr;
  try{
    new_node = new BNode;
  }
  catch (std::bad_alloc& e){
    throw BListException(BListException::E_NO_MEMORY, e.what());
  }

  int half = node->count / 2;
  int remain = node->count - half;

  for (int i = half; i < node->count; ++i){
    new_node->values[i - half] = node->values[i];
  }
  new_node->count = remain;
  node->count = half;

  new_node->next = node->next;
  new_node->prev = node;

  if (node->next){
    node->next->prev = new_node;
  }else{
    tail_ = new_node;
  }
  node->next = new_node;

  ++stats_.NodeCount;

  return new_node;
}