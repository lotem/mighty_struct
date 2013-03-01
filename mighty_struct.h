// License: WTFPL
// GONG Chen  2013-02-28

#ifndef MIGHTY_STRUCT_H_
#define MIGHTY_STRUCT_H_

#include <cstdlib>
#include <cstring>
#include <string>
#include <stdint.h>

typedef /*size_t*/uint16_t Size;

// Limitation: cannot point to itself
// (zero is used to represent NULL pointer)
template <class T = char, class Offset = /*intptr_t*/int16_t>
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
  String& operator= (const char* str) { data = str; return *this; }
  const char* c_str() const { return data.get(); }
  Size length() const { return c_str() ? strlen(c_str()) : 0; }
  bool empty() const { return !data || !data[0]; }
};

struct WString {
  OffsetPtr<wchar_t> data;
  WString& operator= (const wchar_t* str) { data = str; return *this; }
  const wchar_t* c_str() const { return data.get(); }
  Size length() const { return c_str() ? wcslen(c_str()) : 0; }
  bool empty() const { return !data || !data[0]; }
};

template <class T>
struct Array {
  Size size;
  T at[1];
  bool empty() const { return size == 0; }
  T* begin() { return &at[0]; }
  T* end() { return &at[0] + size; }
  const T* begin() const { return &at[0]; }
  const T* end() const { return &at[0] + size; }
};

template <class T>
struct Vector {
  OffsetPtr<Array <T> > arr;
  Vector<T>& operator= (Array<T>* a) { arr = a; return *this; }
  T& operator[] (int index) { return arr->at[index]; }
  const T& operator[] (int index) const { return arr->at[index]; }
  Size size() const { return arr ? arr->size : 0; }
  bool empty() const { return !arr || arr->empty(); }
  T* begin() { return arr->begin(); }
  T* end() { return arr->end(); }
  const T* begin() const { return arr->begin(); }
  const T* end() const { return arr->end(); }
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

  const char* CreateString(const std::string &str);
  const wchar_t* CreateWString(const std::wstring &str);
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
      t[i].template Init<T>();
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

inline const char* Struct::CreateString(const std::string &str) {
  size_t size = str.length() + 1;
  char* p = Allocate<char>(size);
  if (p)
    strncpy(p, str.c_str(), size);
  return p;
}

inline const wchar_t* Struct::CreateWString(const std::wstring &str) {
  size_t size = str.length() + 1;
  wchar_t* p = Allocate<wchar_t>(size);
  if (p)
    wcsncpy(p, str.c_str(), size);
  return p;
}

#endif  // MIGHTY_STRUCT_H_
