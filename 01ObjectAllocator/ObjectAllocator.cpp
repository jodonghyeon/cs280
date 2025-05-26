/*!
@file ObjectAllocator.cpp
@author Donghyeon Jo (donghyeon.jo)
@assignment ObjectAllocator
@course cs280 Spring
@date May 14, 2025
*//*______________________________________________________________________*/

#include "ObjectAllocator.h"
#include <string.h>

ObjectAllocator::ObjectAllocator(size_t ObjectSize, const OAConfig& config) :
  PageList_(nullptr), FreeList_(nullptr), config_(config)
{
  if (config_.Alignment_){
    config_.LeftAlignSize_ = static_cast<unsigned>((config_.Alignment_ - ((sizeof(void*) + config_.HBlockInfo_.size_ + config_.PadBytes_) % config_.Alignment_)) % config_.Alignment_);
    config_.InterAlignSize_ = static_cast<unsigned>((config_.Alignment_ - ((sizeof(void*) + config_.LeftAlignSize_ + 2 * config_.HBlockInfo_.size_ + 3 * config_.PadBytes_ + ObjectSize) % config_.Alignment_)) % config_.Alignment_);
  }

  stats_.ObjectSize_ = ObjectSize;
  inter_block_size_ = static_cast<unsigned>(config_.InterAlignSize_ + config_.HBlockInfo_.size_ + config_.PadBytes_ * 2 + stats_.ObjectSize_);
  stats_.PageSize_ = sizeof(void*) + config_.LeftAlignSize_+ inter_block_size_ * config_.ObjectsPerPage_ - config_.InterAlignSize_;

  allocate_new_page();
}

ObjectAllocator::~ObjectAllocator(){
  GenericObject *page = PageList_;

  while (page){
    if (config_.HBlockInfo_.type_ == OAConfig::hbExternal){
      unsigned char *block = reinterpret_cast<unsigned char*>(page) + sizeof(void*) + config_.LeftAlignSize_ + config_.HBlockInfo_.size_ + config_.PadBytes_;
      for (size_t i = 0; i < config_.ObjectsPerPage_; ++i){
        reset_header(reinterpret_cast<GenericObject*>(block + inter_block_size_ * i));
      }
    }
    GenericObject *temp = page;
    page = page->Next;
    delete[] reinterpret_cast<unsigned char *>(temp);
  }
}

void *ObjectAllocator::Allocate(const char *label){
  if (!FreeList_ ){
    allocate_new_page();
  }

  ++stats_.Allocations_;
  --stats_.FreeObjects_;
  ++stats_.ObjectsInUse_;
  if (stats_.ObjectsInUse_ > stats_.MostObjects_){
    stats_.MostObjects_ = stats_.ObjectsInUse_;
  }

  GenericObject *object = FreeList_;
  pop_front(false);

  set_header(object, label);

  memset(object, ALLOCATED_PATTERN, stats_.ObjectSize_);

  return object;
}

void ObjectAllocator::Free(void *Object){
  if (config_.DebugOn_){
    if (!validate_object(Object)){
      throw OAException(OAException::E_BAD_BOUNDARY, "Free: Object not on a boundary.");
    }
    
    if (is_freed(Object)){
      throw OAException(OAException::E_MULTIPLE_FREE, "Free: Object has already been freed.");
    }

    if (is_corrupted(Object)){
      throw OAException(OAException::E_CORRUPTED_BLOCK, "Free: Object has been corrupted");
    }
  }

  ++stats_.Deallocations_;
  ++stats_.FreeObjects_;
  --stats_.ObjectsInUse_;

  memset(Object, FREED_PATTERN, stats_.ObjectSize_);

  reset_header(reinterpret_cast<GenericObject*>(Object));

  push_front(false, reinterpret_cast<GenericObject*>(Object));
}

unsigned ObjectAllocator::DumpMemoryInUse(DUMPCALLBACK fn) const{
  const GenericObject *page = PageList_;

  while (page){
    const unsigned char *block = reinterpret_cast<const unsigned char*>(page) + sizeof(void*) + config_.LeftAlignSize_ + config_.HBlockInfo_.size_ + config_.PadBytes_;
    for (size_t i = 0; i < config_.ObjectsPerPage_; ++i){
      fn(block + inter_block_size_ * i, stats_.ObjectSize_);
    }
    page = page->Next;
  }
  return stats_.ObjectsInUse_;
}

unsigned ObjectAllocator::ValidatePages(VALIDATECALLBACK fn) const{
  if (!config_.DebugOn_ || config_.PadBytes_ == 0)
    return 0;

  unsigned corrupted_blocks_count = 0;
  const GenericObject *page = PageList_;

  while (page){
    const unsigned char *block = reinterpret_cast<const unsigned char*>(page) + sizeof(void*) + config_.LeftAlignSize_ + config_.HBlockInfo_.size_ + config_.PadBytes_;
    for (size_t i = 0; i < config_.ObjectsPerPage_; ++i){
      fn(block + inter_block_size_ * i, stats_.ObjectSize_);
      if (is_corrupted(block + inter_block_size_ * i))
        ++ corrupted_blocks_count;
    }
    page = page->Next;
  }
  return corrupted_blocks_count;
}

unsigned ObjectAllocator::FreeEmptyPages(){
  return 0;
}

bool ObjectAllocator::ImplementedExtraCredit(){
  return false;
}

void ObjectAllocator::SetDebugState(bool State){
  config_.DebugOn_ = State;
}

const void *ObjectAllocator::GetFreeList() const{
  return FreeList_;
}

const void *ObjectAllocator::GetPageList() const{
  return PageList_;
}

OAConfig ObjectAllocator::GetConfig() const{
  return config_;
}

OAStats ObjectAllocator::GetStats() const{
  return stats_;
}





// Helper Methods

bool ObjectAllocator::validate_object(const void *object) const{
  if (!object)
    return false;

  const unsigned char *object_ptr = reinterpret_cast<const unsigned char*>(object);
  const GenericObject* page = PageList_;
  while (!(object_ptr >= reinterpret_cast<const unsigned char*>(page) && object_ptr < reinterpret_cast<const unsigned char*>(page) + stats_.PageSize_)){
    if (!page)
      return false;
    page = page->Next;
  }
  
  const unsigned char *page_first_block = reinterpret_cast<const unsigned char*>(page) + sizeof(void*) + config_.LeftAlignSize_ + config_.HBlockInfo_.size_ + config_.PadBytes_;
  if ((object_ptr - page_first_block) % inter_block_size_ != 0)
    return false;
  
  return true;
}

bool ObjectAllocator::is_freed(const void *object) const{
  switch (config_.HBlockInfo_.type_)
  {
  case OAConfig::HBLOCK_TYPE::hbNone: {
    const GenericObject *freed_object = FreeList_;
    while (freed_object){
      if (object == static_cast<const void*>(freed_object))
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

bool ObjectAllocator::is_corrupted(const void *object) const{
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

GenericObject *ObjectAllocator::find_previous(bool is_page_list, const GenericObject *elem) const{
  GenericObject *previous = nullptr;
  GenericObject *current = is_page_list ? PageList_ : FreeList_;
  while (current){
    if (current == elem){
      return previous;
    }
    previous = current;
    current = current->Next;
  }
  return nullptr;
}

void ObjectAllocator::push_front(bool is_page_list, GenericObject *elem){
  if (is_page_list){
    elem->Next = PageList_;
    PageList_ = elem;
  }else{
    elem->Next = FreeList_;
    FreeList_ = elem;
  }
}

void ObjectAllocator::pop_front(bool is_page_list){
  if (is_page_list){
    PageList_ = PageList_->Next;
  }else{
    FreeList_ = FreeList_->Next;
  }
}

void ObjectAllocator::remove_next(GenericObject *previous){
  previous->Next = previous->Next->Next;
}

void ObjectAllocator::allocate_new_page(){
  if (stats_.PagesInUse_ >= config_.MaxPages_ && config_.MaxPages_ != 0){
    throw OAException(OAException::E_NO_PAGES, "allocate_new_page: Exceeded maximum number of pages.");
  }
  
  unsigned char *newpage;
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
    while (*(ptr++)){
      ++len;
    }

    char *new_label;
    try {
      new_label = new char[len + 1];
      *reinterpret_cast<MemBlockInfo**>(header) = new MemBlockInfo;
    }
    catch (std::bad_alloc&){
      throw OAException(OAException::E_NO_MEMORY, "set_header: No system memory available.");
    }

    for (size_t i = 0; i < len + 1; i++){
      new_label[i] = label[i];
    }
    (*reinterpret_cast<MemBlockInfo**>(header))->in_use = true;
    (*reinterpret_cast<MemBlockInfo**>(header))->label = new_label;
    (*reinterpret_cast<MemBlockInfo**>(header))->alloc_num = stats_.Allocations_;
    break;
  }
}

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
    if (reinterpret_cast<MemBlockInfo*>(header) && reinterpret_cast<MemBlockInfo*>(header)->label)
      delete[] reinterpret_cast<MemBlockInfo*>(header)->label;
    if (reinterpret_cast<MemBlockInfo*>(header))
      delete reinterpret_cast<MemBlockInfo*>(header);
    memset(header, 0x00, OAConfig::EXTERNAL_HEADER_SIZE);
    break;
  }
}