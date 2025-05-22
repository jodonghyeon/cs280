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
  if (config.Alignment_){
    config_.LeftAlignSize_ = static_cast<unsigned>(config.Alignment_ - sizeof(void*) - config.HBlockInfo_.size_ - config.PadBytes_);
    config_.InterAlignSize_ = static_cast<unsigned>(config.Alignment_ - ((ObjectSize + 2 * config.PadBytes_ + config.HBlockInfo_.size_) % config.Alignment_));
  }

  stats_.ObjectSize_ = ObjectSize;
  stats_.PageSize_ = sizeof(void*) + config_.LeftAlignSize_+ config_.ObjectsPerPage_ * (config_.HBlockInfo_.size_ + 2 * config_.PadBytes_ + ObjectSize) + (config_.ObjectsPerPage_ - 1) * config_.InterAlignSize_;
  
  AllocateNewPage();
}

ObjectAllocator::~ObjectAllocator(){
  
}

void *ObjectAllocator::Allocate(const char *label){
  if (!FreeList_ && config_.MaxPages_){
    throw OAException(OAException::E_NO_PAGES, "allocate_new_page: Exceeded maximum number of pages");
    
    AllocateNewPage();
  }

  unsigned char *temp = reinterpret_cast<unsigned char*>(FreeList_);
  FreeList_ = FreeList_->Next;

  unsigned char *header = reinterpret_cast<unsigned char*>(temp) - config_.PadBytes_ - config_.HBlockInfo_.size_;

  switch (config_.HBlockInfo_.type_)
  {
  case OAConfig::HBLOCK_TYPE::hbNone:
    break;
  case OAConfig::HBLOCK_TYPE::hbBasic:
    *reinterpret_cast<unsigned int*>(header) = stats_.Allocations_;
    *(header + sizeof(unsigned int)) = 0b00000001;
    break;
  case OAConfig::HBLOCK_TYPE::hbExtended:
    header += config_.HBlockInfo_.additional_;
    ++*reinterpret_cast<unsigned short*>(header);
    header += sizeof(unsigned short);
    *reinterpret_cast<unsigned int*>(header) = stats_.Allocations_;
    *(header + sizeof(unsigned int)) = 0b00000001;
    break;
  case OAConfig::HBLOCK_TYPE::hbExternal:
    const char *ptr = label;
    size_t len = 0;
    while (*(ptr++))
      ++len;
    char *new_label = new char[len + 1];
    for (size_t i = 0; i < len + 1; i++){
      new_label[i] = label[i];
    }
    *reinterpret_cast<MemBlockInfo**>(header) = new MemBlockInfo;
    reinterpret_cast<MemBlockInfo*>(*reinterpret_cast<MemBlockInfo**>(header))->in_use = true;
    reinterpret_cast<MemBlockInfo*>(*reinterpret_cast<MemBlockInfo**>(header))->label = new_label;
    reinterpret_cast<MemBlockInfo*>(*reinterpret_cast<MemBlockInfo**>(header))->alloc_num = stats_.Allocations_;
    break;
  }

  memset(temp, ALLOCATED_PATTERN, stats_.ObjectSize_ );

  ++stats_.Allocations_;
  --stats_.FreeObjects_;
  ++stats_.ObjectsInUse_;
  if (stats_.ObjectsInUse_ > stats_.MostObjects_){
    stats_.MostObjects_ = stats_.ObjectsInUse_;
  }

  return temp;
}

void ObjectAllocator::Free(void *Object){
  (void)Object;
}

unsigned ObjectAllocator::DumpMemoryInUse(DUMPCALLBACK fn) const{
  (void)fn;
  return 0;
}

unsigned ObjectAllocator::ValidatePages(VALIDATECALLBACK fn) const{
  (void)fn;
  return 0;
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


void ObjectAllocator::AllocateNewPage(){
  unsigned char *newpage;
  try {
    newpage = new unsigned char[stats_.PageSize_];
  }
  catch (std::bad_alloc&){
    throw OAException(OAException::E_NO_MEMORY, "allocate_new_page: No system memory available.");
  }

  GenericObject *temp1 = reinterpret_cast<GenericObject*>(newpage);
  temp1->Next = PageList_;
  PageList_ = temp1;
  newpage += sizeof(void*);

  if (config_.DebugOn_)
    memset(newpage, ALIGN_PATTERN, config_.LeftAlignSize_);
  newpage += config_.LeftAlignSize_;

  memset(newpage, 0x00, config_.HBlockInfo_.size_);
  newpage += config_.HBlockInfo_.size_;

  if (config_.DebugOn_)
    memset(newpage, PAD_PATTERN, config_.PadBytes_);
  newpage += config_.PadBytes_ ;

  GenericObject *temp2 = reinterpret_cast<GenericObject*>(newpage);
  temp2->Next = FreeList_;
  FreeList_ = temp2;
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

    GenericObject *temp3 = reinterpret_cast<GenericObject*>(newpage);
    temp3->Next = FreeList_;
    FreeList_ = temp3;
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

// void ObjectAllocator::ResetHeader(char *address) {
//   switch (config_.HBlockInfo_.type_)
//   {
//   case OAConfig::HBLOCK_TYPE::hbNone:
//     return;
//   case OAConfig::HBLOCK_TYPE::hbBasic:
//     memset(address, 0x00, config_.HBlockInfo_.size_);
//     return;
//   case OAConfig::HBLOCK_TYPE::hbExtended:
//     memset(address, 0x00, config_.HBlockInfo_.size_);
//     break;
//   case OAConfig::HBLOCK_TYPE::hbExternal:
//   }
// }