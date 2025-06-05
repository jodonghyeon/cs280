/*!
@file SotedList.h
@author Donghyeon Jo (donghyeon.jo)
@assignment SortedList
@course cs280 Spring
@date May 31, 2025
*//*______________________________________________________________________*/

//---------------------------------------------------------------------------
#ifndef SORTEDLISTH
#define SORTEDLISTH
//---------------------------------------------------------------------------

#include <string>     // error strings
#include <functional> // std::less

#include "ObjectAllocator.h"

/*!
  The exception class for the SortedList
*/
class SortedListException : public std::exception
{
  private:  
    int m_ErrCode;             //!< The specific error code
    std::string m_Description; //!< Human-readable string

  public:
    /*!
      Constructor

      \param ErrCode
        The error code which is either E_NO_MEMORY or E_CLIENT_ERROR

      \param Description
        The human-readable string
    */
    SortedListException(int ErrCode, const std::string& Description) :
    m_ErrCode(ErrCode), m_Description(Description) {};

    /*!
      Get the code from the exception object.

      \return
        The error code which is either E_NO_MEMORY or E_CLIENT_ERROR
    */
    virtual int code() const { 
      return m_ErrCode; 
    }

    /*!
      Get the human-readable string from the exception object

      \return
        The human-readable string
    */
    virtual const char *what() const noexcept {
      return m_Description.c_str();
    }

    /*!
      Destructor
    */
    virtual ~SortedListException() noexcept {
    }

    /*!
      The two kinds of exceptions.
      E_NO_MEMORY    - No memory (from ObjectAllocator)
      E_CLIENT_ERROR - Other error from client
    */
    enum SORTEDLIST_EXCEPTION {E_NO_MEMORY, E_CLIENT_ERROR};
};

//! A templatized linked-list that can be sorted
template <typename T, typename Pred = std::less<T> >
class SortedList
{
  struct Node; // Forward decl for the private Node
  public:
      // Constructor
    SortedList(Pred sorter = std::less<T>(), ObjectAllocator *Allocator = 0, 
               bool SharedAllocator = false);

      // Copy constructor
    SortedList(const SortedList &rhs);

      // Destructor
    ~SortedList();
      
      // Assignment operator
    SortedList& operator=(const SortedList &rhs);

      // Subscript operator (only need one)
    const T& operator[](size_t index) const; 

    void push_back(const T& value); // add to end
    void insert(const T& value);    // insert at proper (sorted) position
    unsigned size() const;          // number of nodes
    void clear();                   // free all nodes

      // Sorts using the selection sort algorithm (N^2)
    template <typename Sorter>
    void selection_sort(Sorter sorter = std::less<T>()); 

      // Sorts using the merge sort algorithm (NlgN)
    template <typename Sorter>
    void merge_sort(Sorter sorter = std::less<T>());

      // If the extra credit has been implemented or not
    static bool ImplementedExtraCredit();

      // The sizeof a node
    static unsigned nodesize();

  private:
    //! The node structure used in the SortedList
    struct Node
    {
      Node *Next; //!< Pointer to the next node
      Node *Prev; //!< Pointer to the previous node
      T Data;     //!<  The payload
      /*!
        Constructor

        \param data
          The data to put into the node
      */
      Node(const T& data) : Data(data) {}
    };

      // Some "suggested" private data
      
    Node *head_;    //!< the real head
    Node *tail_;    //!< the real tail
    unsigned size_; //!< count of nodes

      // Don't swap these
    ObjectAllocator *objAllocator_; //!< The OA to use/share
    bool freeAllocator_;            //!< Do we own it, then free it.
    bool shareAllocator_;           //!< Share it with copies?
    
    // Other private data and helper functions...

    Pred sorter_;                   //!< Setted sorter (using at insert method)
    
    void swap(Node *a, Node *b);    // Swaps two nodes in the list.
};

#include "SortedList.cpp"

#endif