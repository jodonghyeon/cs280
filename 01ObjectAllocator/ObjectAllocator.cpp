/*!
@file ObjectAllocator.cpp
@author Donghyeon Jo (donghyeon.jo)
@assignment ObjectAllocator
@course cs280 Spring
@date May 26, 2025
*//*______________________________________________________________________*/

#include "ObjectAllocator.h"
#include <string.h>



// -----------------------Public Methods--------------------------


/*!
@brief
  Constructor of ObjectAllocator
  Creates the ObjectManager per the specified values
  Throws an exception if the construction fails. (Memory allocation problem)
@param
  ObjectSize - Size of one object
  config - Configuration of ObjectAllocator
*//*______________________________________________________________*/
ObjectAllocator::ObjectAllocator(size_t ObjectSize, const OAConfig& config) :
  PageList_(nullptr), FreeList_(nullptr), object_list_(nullptr), config_(config)
{
  if (config_.Alignment_){
    config_.LeftAlignSize_ = static_cast<unsigned>((config_.Alignment_ - ((sizeof(void*) + config_.HBlockInfo_.size_ + config_.PadBytes_) % config_.Alignment_)) % config_.Alignment_);
    config_.InterAlignSize_ = static_cast<unsigned>((config_.Alignment_ - ((sizeof(void*) + config_.LeftAlignSize_ + 2 * config_.HBlockInfo_.size_ + 3 * config_.PadBytes_ + ObjectSize) % config_.Alignment_)) % config_.Alignment_);
  }

  stats_.ObjectSize_ = ObjectSize;
  left_offset_ = static_cast<unsigned>(sizeof(void*) + config_.LeftAlignSize_ + config_.HBlockInfo_.size_ + config_.PadBytes_);
  inter_block_size_ = static_cast<unsigned>(config_.InterAlignSize_ + config_.HBlockInfo_.size_ + config_.PadBytes_ * 2 + stats_.ObjectSize_);
  stats_.PageSize_ = sizeof(void*) + config_.LeftAlignSize_+ inter_block_size_ * config_.ObjectsPerPage_ - config_.InterAlignSize_;

  if (!config_.UseCPPMemManager_)
    allocate_new_page();
}

/*!
@brief
  Destructor of ObjectAllocator
  Destroys the ObjectManager (never throws)
*//*______________________________________________________________*/
ObjectAllocator::~ObjectAllocator(){
  if (!config_.UseCPPMemManager_){
    GenericObject *page = PageList_;

    while (page){
      if (config_.HBlockInfo_.type_ == OAConfig::hbExternal){
        unsigned char *block = reinterpret_cast<unsigned char*>(page) + left_offset_;
        for (size_t i = 0; i < config_.ObjectsPerPage_; ++i){
          reset_header(reinterpret_cast<GenericObject*>(block + inter_block_size_ * i));
        }
      }
      GenericObject *temp = page;
      page = page->Next;
      delete[] reinterpret_cast<unsigned char *>(temp);
    }
  }else{
    ObjectList *object = object_list_;

    while (object){
      if (object->Object)
        delete[] object->Object;

      ObjectList *temp = object;
      object = object->Next;
      delete temp;
    }
  }
}

/*!
@brief 
  Take an object from the free list and give it to the client (simulates new)
  Throws an exception if the object can't be allocated. (Memory allocation problem)
@param
label - If the header type of configuration is 'External', A null-terminated string to be allocated to the header.
@return Allocated object
*//*______________________________________________________________*/
void *ObjectAllocator::Allocate(const char *label){
  if (!config_.UseCPPMemManager_ && !FreeList_ ){
    allocate_new_page();
  }

  ++stats_.Allocations_;
  ++stats_.ObjectsInUse_;
  if (stats_.ObjectsInUse_ > stats_.MostObjects_){
    stats_.MostObjects_ = stats_.ObjectsInUse_;
  }


  if (!config_.UseCPPMemManager_){
    --stats_.FreeObjects_;
    GenericObject *object;
    object = FreeList_;
    pop_front(false);

    set_header(object, label);

    memset(object, ALLOCATED_PATTERN, stats_.ObjectSize_);
    return object;
  }else{
    ObjectList *new_node = nullptr;
    unsigned char *new_object = nullptr;
    try
    {
      new_node = new ObjectList;
      new_object = new unsigned char[stats_.ObjectSize_];
    }
    catch (std::bad_alloc&){
      throw OAException(OAException::E_NO_MEMORY, "Allocate: No system memory available.");
    }
    
    if (config_.DebugOn_)
      memset(new_object, ALLOCATED_PATTERN, stats_.ObjectSize_);

    new_node->Next = object_list_;
    new_node->Object = new_object;
    object_list_ = new_node;

    return new_object;
  }
}

/*!
@brief
  Returns an object to the free list for the client (simulates delete)
  Throws an exception if the the object can't be freed. (Invalid object)
@param
  Object - Object to be deallocated
*//*______________________________________________________________*/
void ObjectAllocator::Free(void *Object){
  if (!config_.UseCPPMemManager_){
    if (config_.DebugOn_){
      if (!validate_object(reinterpret_cast<const GenericObject*>(Object))){
        throw OAException(OAException::E_BAD_BOUNDARY, "Free: Object not on a boundary.");
      }
      
      if (is_freed(reinterpret_cast<const GenericObject*>(Object))){
        throw OAException(OAException::E_MULTIPLE_FREE, "Free: Object has already been freed.");
      }

      if (is_corrupted(reinterpret_cast<const GenericObject*>(Object))){
        throw OAException(OAException::E_CORRUPTED_BLOCK, "Free: Object has been corrupted");
      }
    }
    
    ++stats_.Deallocations_;
    ++stats_.FreeObjects_;
    --stats_.ObjectsInUse_;

    memset(Object, FREED_PATTERN, stats_.ObjectSize_);

    reset_header(reinterpret_cast<GenericObject*>(Object));

    push_front(false, reinterpret_cast<GenericObject*>(Object));
    
  }else{
    ++stats_.Deallocations_;
    --stats_.ObjectsInUse_;

    if (!object_list_ || !Object){
      if (config_.DebugOn_){
        throw OAException(OAException::E_BAD_BOUNDARY, "Free: Object not on a boundary.");
      }
      return;
    }

    ObjectList *previous = nullptr;
    ObjectList *current = object_list_;
    while (current){
      if (current->Object == reinterpret_cast<unsigned char *>(Object)){
        break;
      }
      previous = current;
      current = current->Next;
    }

    if (!current){
      if (config_.DebugOn_){
        throw OAException(OAException::E_BAD_BOUNDARY, "Free: Object not on a boundary.");
      }
      return;
    }

    if (previous){
      previous->Next = current->Next;
    }else{
      object_list_ = object_list_->Next;
    }

    delete[] current->Object;
    delete current;

  }
}

/*!
@brief
  Calls the callback fn for each block still in use
@param
  fn - The callback function
@return Number of objects in use
*//*______________________________________________________________*/
unsigned ObjectAllocator::DumpMemoryInUse(DUMPCALLBACK fn) const{
  const GenericObject *page = PageList_;

  while (page){
    const unsigned char *block = reinterpret_cast<const unsigned char*>(page) + left_offset_;
    for (size_t i = 0; i < config_.ObjectsPerPage_; ++i){
      if (!is_freed(reinterpret_cast<const GenericObject*>(block + inter_block_size_ * i)))
        fn(block + inter_block_size_ * i, stats_.ObjectSize_);
    }
    page = page->Next;
  }
  return stats_.ObjectsInUse_;
}

/*!
@brief
  Calls the callback fn for each block that is potentially corrupted
@param
  fn - The callback function
@return Number of corrupted blocks
*//*______________________________________________________________*/
unsigned ObjectAllocator::ValidatePages(VALIDATECALLBACK fn) const{
  if (!config_.DebugOn_ || config_.PadBytes_ == 0)
    return 0;

  unsigned corrupted_blocks_count = 0;
  const GenericObject *page = PageList_;

  while (page){
    const unsigned char *block = reinterpret_cast<const unsigned char*>(page) + left_offset_;
    for (size_t i = 0; i < config_.ObjectsPerPage_; ++i){
      if (is_corrupted(reinterpret_cast<const GenericObject*>(block + inter_block_size_ * i))){
        fn(block + inter_block_size_ * i, stats_.ObjectSize_);
        ++ corrupted_blocks_count;
      }
    }
    page = page->Next;
  }
  return corrupted_blocks_count;
}

/*!
@brief
  Frees all empty pages (extra credit)
@return
  0 (Not implemented yet)
*//*______________________________________________________________*/
unsigned ObjectAllocator::FreeEmptyPages(){
  return 0;
}

/*!
@brief
  Returns true if FreeEmptyPages and alignments are implemented
@return
  false (Not implemented extra credit method)
*//*______________________________________________________________*/
bool ObjectAllocator::ImplementedExtraCredit(){
  return false;
}

/*!
@brief
  Set debug state
@param
  State - State to set (true=enable, false=disable)
*//*______________________________________________________________*/
void ObjectAllocator::SetDebugState(bool State){
  config_.DebugOn_ = State;
}

/*!
@brief
  Returns a pointer to the internal free list
@return A pointer to the internal free list
*//*______________________________________________________________*/
const void *ObjectAllocator::GetFreeList() const{
  return FreeList_;
}

/*!
@brief
  Returns a pointer to the internal page list
@return A pointer to the internal page list
*//*______________________________________________________________*/
const void *ObjectAllocator::GetPageList() const{
  return PageList_;
}

/*!
@brief
  Returns the configuration parameters
@return The configuration of ObjectAllocator
*//*______________________________________________________________*/
OAConfig ObjectAllocator::GetConfig() const{
  return config_;
}

/*!
@brief
  Returns the statistics for the allocator
@return The statistics for the allocator
*//*______________________________________________________________*/
OAStats ObjectAllocator::GetStats() const{
  return stats_;
}




// -----------------------Helper Methods--------------------------


/*!
@brief
  Checks if the given object pointer is a valid address
@param
  object - The object pointer to check
@return Result of validation
*//*______________________________________________________________*/
bool ObjectAllocator::validate_object(const GenericObject *object) const{
  if (!object)
    return false;

  const unsigned char *object_ptr = reinterpret_cast<const unsigned char*>(object);
  const GenericObject* page = PageList_;
  while (!(object_ptr >= reinterpret_cast<const unsigned char*>(page) && object_ptr < reinterpret_cast<const unsigned char*>(page) + stats_.PageSize_)){
    if (!page)
      return false;
    page = page->Next;
  }
  
  const unsigned char *page_first_block = reinterpret_cast<const unsigned char*>(page) + left_offset_;
  if ((object_ptr - page_first_block) % inter_block_size_ != 0)
    return false;
  
  return true;
}

/*!
@brief
  Checks if the given object is in the FreeList of ObjectAllocator
@param
  object - The object to check
@return Checked result
*//*______________________________________________________________*/
bool ObjectAllocator::is_freed(const GenericObject *object) const{
  switch (config_.HBlockInfo_.type_)
  {
  case OAConfig::HBLOCK_TYPE::hbNone: {
    const GenericObject *freed_object = FreeList_;
    while (freed_object){
      if (object == freed_object)
        return true;
      freed_object = freed_object->Next;
    }
    return false;
  }
  case OAConfig::HBLOCK_TYPE::hbBasic:
  case OAConfig::HBLOCK_TYPE::hbExtended: {
    if (*(reinterpret_cast<const unsigned char*>(object) - config_.PadBytes_ - 1))
      return false;
    else
      return true;
  }
  case OAConfig::HBLOCK_TYPE::hbExternal:
    if (*reinterpret_cast<MemBlockInfo* const*>(reinterpret_cast<const unsigned char*>(object) - config_.PadBytes_ - OAConfig::EXTERNAL_HEADER_SIZE))
      return false;
    else
      return true;
  }
  return false;
}

/*!
@brief
  Checks if the pad of the block the object belongs to is corrupted
@param
  The object to check
@return Checked result
*//*______________________________________________________________*/
bool ObjectAllocator::is_corrupted(const GenericObject *object) const{
  if (!config_.DebugOn_ || config_.PadBytes_ == 0)
    return false;

  const unsigned char *casted = reinterpret_cast<const unsigned char*>(object);

  for (size_t i = 0; i < config_.PadBytes_; ++i){
    if (*(casted - i - 1) != PAD_PATTERN)
      return true;
    if (*(casted + stats_.ObjectSize_ + i) != PAD_PATTERN)
      return true;
  }
  return false;
}

/*!
@brief
  Adds a given object to the PageList/FreeList of ObjectAllocator
@param
  is_page_list - Determine the list to which objects should be added (true=PageList, false=FreeList)
  object - Object to add
*//*______________________________________________________________*/
void ObjectAllocator::push_front(bool is_page_list, GenericObject *object){
  if (is_page_list){
    object->Next = PageList_;
    PageList_ = object;
  }else{
    object->Next = FreeList_;
    FreeList_ = object;
  }
}

/*!
@brief
  Remove the first object from PageList/FreeList
@param
  is_page_list - Determine the list to which objects should be removed (true=PageList, false=FreeList)
*//*______________________________________________________________*/
void ObjectAllocator::pop_front(bool is_page_list){
  if (is_page_list){
    PageList_ = PageList_->Next;
  }else{
    FreeList_ = FreeList_->Next;
  }
}

/*!
@brief
  Allocate a new page
*//*______________________________________________________________*/
void ObjectAllocator::allocate_new_page(){
  if (stats_.PagesInUse_ >= config_.MaxPages_ && config_.MaxPages_ != 0){
    throw OAException(OAException::E_NO_PAGES, "allocate_new_page: Exceeded maximum number of pages.");
  }
  
  unsigned char *newpage = nullptr;
  try {
    newpage = new unsigned char[stats_.PageSize_];
  }
  catch (std::bad_alloc&){
    throw OAException(OAException::E_NO_MEMORY, "allocate_new_page: No system memory available.");
  }

  push_front(true, reinterpret_cast<GenericObject*>(newpage));
  newpage += sizeof(void*);

  if (config_.DebugOn_)
    memset(newpage, ALIGN_PATTERN, config_.LeftAlignSize_);
  newpage += config_.LeftAlignSize_;

  memset(newpage, 0x00, config_.HBlockInfo_.size_);
  newpage += config_.HBlockInfo_.size_;

  if (config_.DebugOn_)
    memset(newpage, PAD_PATTERN, config_.PadBytes_);
  newpage += config_.PadBytes_ ;

  push_front(false, reinterpret_cast<GenericObject*>(newpage));
  newpage += sizeof(void*);

  if (config_.DebugOn_)
    memset(newpage, UNALLOCATED_PATTERN, stats_.ObjectSize_ - sizeof(void*));
  newpage += (stats_.ObjectSize_ - sizeof(void*));

  if (config_.DebugOn_)
    memset(newpage, PAD_PATTERN, config_.PadBytes_);
  newpage +=  config_.PadBytes_ ;

  for (size_t i = 0; i < config_.ObjectsPerPage_ - 1; i++){
    if (config_.DebugOn_)
      memset(newpage, ALIGN_PATTERN, config_.InterAlignSize_);
    newpage += config_.InterAlignSize_;

    memset(newpage, 0x00, config_.HBlockInfo_.size_);
    newpage += config_.HBlockInfo_.size_;

    if (config_.DebugOn_)
      memset(newpage, PAD_PATTERN, config_.PadBytes_);
    newpage += config_.PadBytes_ ;

    push_front(false, reinterpret_cast<GenericObject*>(newpage));
    newpage += sizeof(void*);

    if (config_.DebugOn_)
      memset(newpage, UNALLOCATED_PATTERN, stats_.ObjectSize_ - sizeof(void*));
    newpage += (stats_.ObjectSize_ - sizeof(void*));

    if (config_.DebugOn_)
      memset(newpage, PAD_PATTERN, config_.PadBytes_);
    newpage += config_.PadBytes_ ;
  }

  ++stats_.PagesInUse_;
  stats_.FreeObjects_ += config_.ObjectsPerPage_;
}

/*!
@brief
  Sets the header that matches the configuration of the block to which the given object belongs (used when allocating)
@param
  object - Object of the block to set the header
  label - If the header type of configuration is 'External', A null-terminated string to be allocated to the header.
*//*______________________________________________________________*/
void ObjectAllocator::set_header(GenericObject *object, const char *label){
  unsigned char *header = reinterpret_cast<unsigned char*>(object) - config_.PadBytes_ - config_.HBlockInfo_.size_;

  switch (config_.HBlockInfo_.type_)
  {
  case OAConfig::HBLOCK_TYPE::hbNone:
    break;
  case OAConfig::HBLOCK_TYPE::hbBasic:
    *reinterpret_cast<unsigned int*>(header) = stats_.Allocations_;
    *(header + sizeof(unsigned int)) = 0b00000001;
    break;
  case OAConfig::HBLOCK_TYPE::hbExtended:
    memset(header, 0x00, config_.HBlockInfo_.additional_);
    header += config_.HBlockInfo_.additional_;
    ++*reinterpret_cast<unsigned short*>(header);
    header += sizeof(unsigned short);
    *reinterpret_cast<unsigned int*>(header) = stats_.Allocations_;
    *(header + sizeof(unsigned int)) = 0b00000001;
    break;
  case OAConfig::HBLOCK_TYPE::hbExternal:
    const char *ptr = label;
    size_t len = 0;
    if (label){
      while (*(ptr++)){
        ++len;
      }
    }

    char *new_label = nullptr;
    try {
      if (label)
        new_label = new char[len + 1];
      *reinterpret_cast<MemBlockInfo**>(header) = new MemBlockInfo;
    }
    catch (std::bad_alloc&){
      throw OAException(OAException::E_NO_MEMORY, "set_header: No system memory available.");
    }

    if (label){
      for (size_t i = 0; i < len + 1; i++){
        new_label[i] = label[i];
      }
    }

    (*reinterpret_cast<MemBlockInfo**>(header))->in_use = true;
    (*reinterpret_cast<MemBlockInfo**>(header))->label = new_label;
    (*reinterpret_cast<MemBlockInfo**>(header))->alloc_num = stats_.Allocations_;
    break;
  }
}

/*!
@brief
  Resets the header that matches the configuration of the block to which the given object belongs (used when Freeing)
@param
  object - Object of the block to reset the header
*//*______________________________________________________________*/
void ObjectAllocator::reset_header(GenericObject *object){
  unsigned char *header = reinterpret_cast<unsigned char*>(object) - config_.PadBytes_ - config_.HBlockInfo_.size_;

  switch (config_.HBlockInfo_.type_)
  {
  case OAConfig::HBLOCK_TYPE::hbNone:
    break;
  case OAConfig::HBLOCK_TYPE::hbBasic:
    memset(header, 0x00, OAConfig::BASIC_HEADER_SIZE);
    break;
  case OAConfig::HBLOCK_TYPE::hbExtended:
    memset(header, 0x00, config_.HBlockInfo_.additional_);
    header += (config_.HBlockInfo_.additional_ + sizeof(unsigned short));
    memset(header, 0x00, sizeof(unsigned int));
    *(header + sizeof(unsigned int)) = 0b00000000;
    break;
  case OAConfig::HBLOCK_TYPE::hbExternal:
    if (*reinterpret_cast<MemBlockInfo**>(header)){
      if ((*reinterpret_cast<MemBlockInfo**>(header))->label)
        delete[] (*reinterpret_cast<MemBlockInfo**>(header))->label;
      delete *reinterpret_cast<MemBlockInfo**>(header);
    }
    memset(header, 0x00, OAConfig::EXTERNAL_HEADER_SIZE);
    break;
  }
}