// License: WTFPL
// GONG Chen  2013-02-28

#ifndef MIGHTY_STRUCT_H_
#define MIGHTY_STRUCT_H_

#include <cstdlib>
#include <cstring>
#include <string>
//#include <stdint.h>

typedef size_t Size;

// Limitation: cannot point to itself
// (zero is used to represent NULL pointer)
template <class T = char, class Offset = intptr_t>
class OffsetPtr {
 public:
  OffsetPtr() : offset_(0) {}
  OffsetPtr(Offset offset) : offset_(offset) {}
  OffsetPtr(const OffsetPtr<T> &ptr)
      : offset_(to_offset(ptr.get())) {}
  OffsetPtr(const T *ptr)
      : offset_(to_offset(ptr)) {}
  OffsetPtr<T>& operator= (const OffsetPtr<T> &ptr) {
    offset_ = to_offset(ptr.get());
    return *this;
  }
  OffsetPtr<T>& operator= (const T *ptr) {
    offset_ = to_offset(ptr);
    return *this;
  }
  operator bool() const {
    return !!offset_;
  }
  T* operator-> () const {
    return get();
  }
  T& operator* () const {
    return *get();
  }
  T& operator[] (size_t index) const {
    return *(get() + index);
  }
  // TODO: define other operations
  T* get() const {
    if (!offset_) return NULL;
    return reinterpret_cast<T*>((char*)&offset_ + offset_);
  }
 private:
  Offset to_offset(const T* ptr) const {
    return ptr ? (char*)ptr - (char*)(&offset_) : 0;
  }
  Offset offset_;
};

struct String {
  OffsetPtr<char> data;
  const char* c_str() const { return data.get(); }
  Size length() const { return c_str() ? strlen(c_str()) : 0; }
  bool empty() const { return !data || !data[0]; }
};

struct WString {
  OffsetPtr<wchar_t> data;
  const wchar_t* c_str() const { return data.get(); }
  Size length() const { return c_str() ? wcslen(c_str()) : 0; }
  bool empty() const { return !data || !data[0]; }
};

template <class T>
struct Array {
  Size size;
  T at[1];
  T* begin() { return &at[0]; }
  T* end() { return &at[0] + size; }
  const T* begin() const { return &at[0]; }
  const T* end() const { return &at[0] + size; }
};

template <class T>
struct List {
  Size size;
  OffsetPtr<T> at;
  T* begin() { return &at[0]; }
  T* end() { return &at[0] + size; }
  const T* begin() const { return &at[0]; }
  const T* end() const { return &at[0] + size; }
};

struct Struct {
  Size struct_size;
  Size capacity;
  Size used_space;

  char* address() const { return (char*)this; }

  template <class S>
  void Init(Size capacity = sizeof(S)) {
    struct_size = used_space = sizeof(S);
    this->capacity = capacity;
  }

  template <class T>
  bool HasMember(const T& member) const {
    return struct_size > (char*)&member - (char*)this;
  }

  template <class S>
  static S* New(Size capacity);
  template <class S>
  static S* InplaceNew(void* buffer, Size capacity);
  template <class S>
  static S* NewCopy(S* src);
  template <class S>
  static S* Copy(S* dest, S* src);
  template <class S>
  static void Delete(S* ptr);

  template <class T>
  T* Allocate(size_t count = 1);

  template <class T>
  T* Find(size_t offset) const;

  template <class T>
  T* Create(size_t count = 1);

  template <class T>
  Array<T>* CreateArray(size_t size);

  template <class T>
  List<T> CreateList(size_t size);

  String CreateString(const std::string &str);
  WString CreateWString(const std::wstring &str);
};

// function definitions

template <class S>
S* Struct::New(Size capacity) {
  return InplaceNew<S>(malloc(capacity), capacity);
}

template <class S>
S* Struct::InplaceNew(void* buffer, Size capacity) {
  S* s = (S*)buffer;
  s->template Init<S>(capacity);
  return s;
}

template <class S>
S* Struct::NewCopy(S* src) {
  if (!src) return NULL;
  S* ret = New<S>(src->used_space);
  if (ret) {
    Copy(ret, src);
  }
  return ret;
}

template <class S>
S* Struct::Copy(S* dest, S* src) {
  if (!dest || !src || dest->capacity < src->used_space)
    return NULL;
  size_t original_capacity = dest->capacity;
  std::memcpy(dest, src, src->used_space);
  dest->capacity = original_capacity;
  return dest;
}

template <class S>
void Struct::Delete(S* ptr) {
  free(ptr);
}

template <class T>
T* Struct::Allocate(size_t count) {
  if (!count)
    return NULL;
  size_t available_space = capacity - used_space;
  size_t required_space = sizeof(T) * count;
  if (required_space > available_space) {
    // not enough space
    return NULL;
  }
  T *ptr = reinterpret_cast<T*>(address() + used_space);
  std::memset(ptr, 0, required_space);
  used_space += required_space;
  return ptr;
}

template <class T>
T* Struct::Find(size_t offset) const {
  if (offset + sizeof(T) > capacity)
    return NULL;
  return reinterpret_cast<T*>(address() + offset);
}

template <class T>
T* Struct::Create(size_t count) {
  T* t = Allocate<T>(count);
  if (t) {
    for (size_t i = 0; i < count; ++i)
      t[i]->template Init<T>();
  }
  return t;
}

template <class T>
Array<T>* Struct::CreateArray(size_t size) {
  size_t num_bytes = sizeof(Array<T>) + sizeof(T) * (size - 1);
  Array<T>* ret = reinterpret_cast<Array<T>*>(Allocate<char>(num_bytes));
  if (!ret)
    return NULL;
  ret->size = size;
  return ret;
}

template <class T>
List<T> Struct::CreateList(size_t size) {
  List<T> list;
  list.at = Allocate<T>(size);
  if (list.at)
    list.size = size;
  return list;
}

inline String Struct::CreateString(const std::string &str) {
  String ret;
  size_t size = str.length() + 1;
  char* p = Allocate<char>(size);
  if (p)
    strncpy(p, str.c_str(), size);
  ret.data = p;
  return ret;
}

inline WString Struct::CreateWString(const std::wstring &str) {
  WString ret;
  size_t size = str.length() + 1;
  wchar_t* p = Allocate<wchar_t>(size);
  if (p)
    wcsncpy(p, str.c_str(), size);
  ret.data = p;
  return ret;
}

#endif  // MIGHTY_STRUCT_H_
