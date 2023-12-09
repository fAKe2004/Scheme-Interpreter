#ifndef SHARED_PTR_H
#define SHARED_PTR_H 1

#include <type_traits>
#include <atomic>

#include "Def.hpp"

#ifndef PARALLEL_OPTIMIZE
typedef int count_type;
#else
typedef std::atomic<int> count_type;
#endif

template <class _Tp>
class SharedPtr {

  _Tp *_ptr;
  count_type *_ref_cnt;
public:
  SharedPtr() 
    : _ptr(nullptr), _ref_cnt(nullptr) {}
  SharedPtr(_Tp *ptr)
    : _ptr(ptr), _ref_cnt(nullptr) {
      if (_ptr)
        _ref_cnt = new count_type(1);
    }
  
  ~SharedPtr() {
    this->destroy();
  }
  
  SharedPtr(const SharedPtr& rhs) : SharedPtr() {
    *this = rhs;
  }
  SharedPtr& operator=(const SharedPtr& rhs) {
    if (this == &rhs)
      return *this;

    this->destroy();
    _ptr = rhs._ptr;
    _ref_cnt = rhs._ref_cnt;
    if (_ref_cnt != nullptr)
      ++*_ref_cnt;
    return *this;
  }

  SharedPtr(SharedPtr&& rhs) : SharedPtr() {
    *this = std::move(rhs);
  }
  SharedPtr& operator=(SharedPtr&& rhs) {
    if (this == &rhs)
      return *this;
    
    this->destroy();
    _ptr = rhs._ptr;
    _ref_cnt = rhs._ref_cnt;
    rhs.invalidate();
    return *this;
  }

  _Tp* get() const {
    return _ptr;
  }
  _Tp& operator*() const {
    return *_ptr;
  }
  _Tp* operator->() const {
    return _ptr;
  }

  // int use_count() const {
  //   return _ref_cnt ? *_ref_cnt : 0; 
  // }

  void reset() {
    reset(nullptr);
  }

  void reset(_Tp *new_ptr) {
    if (_ptr == new_ptr)
      return;
    *this = std::move(SharedPtr(new_ptr));
  }

  operator bool() const {
    return _ptr != nullptr;
  }

private:
  void invalidate() {
    _ptr = nullptr;
    _ref_cnt = nullptr;
  }
  void destroy() {
    if (_ref_cnt == nullptr)
      return;
    --*_ref_cnt;
    if (*_ref_cnt == 0) {
      delete _ptr;
      delete _ref_cnt;
    }
    this->invalidate();
  }
};

template <class _Tp, class ...Args, 
typename = std::enable_if<!std::is_array<_Tp>::value, bool>>
SharedPtr<_Tp> make_shared(Args&&... args) {
  return SharedPtr<_Tp>(new _Tp(std::forward<Args>(args)...));
}


#endif