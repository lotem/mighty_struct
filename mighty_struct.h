// License: WTFPL
// GONG Chen  2013-02-28

#ifndef MIGHTY_STRUCT_H_
#define MIGHTY_STRUCT_H_

#include <cstdlib>
#include <cstring>
#include <stdexcept>
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
  operator T* () const { return get(); }
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
  bool operator== (const char* s) const {
    return data.get() == s || (data && s && !strcmp(data.get(), s));
  }
  bool operator== (const String& o) const { return *this == o.data; }
  bool operator!= (const char* s) const { return !(*this == s); }
  bool operator!= (const String& o) const { return !(*this == o.data); }
  operator std::string () const { return std::string(c_str()); }
  operator const char* () const { return c_str(); }
  const char* c_str() const { return data.get(); }
  Size length() const { return c_str() ? strlen(c_str()) : 0; }
  bool empty() const { return !data || !data[0]; }
  void clear() { data = NULL; }
};

struct WString {
  OffsetPtr<wchar_t> data;
  WString& operator= (const wchar_t* str) { data = str; return *this; }
  bool operator== (const wchar_t* s) const {
    return data.get() == s || (data && s && !wcscmp(data.get(), s));
  }
  bool operator== (const WString& o) const { return *this == o.data; }
  bool operator!= (const wchar_t* s) const { return !(*this == s); }
  bool operator!= (const WString& o) const { return !(*this == o.data); }
  operator std::wstring () const { return std::wstring(c_str()); }
  operator const wchar_t* () const { return c_str(); }
  const wchar_t* c_str() const { return data.get(); }
  Size length() const { return c_str() ? wcslen(c_str()) : 0; }
  bool empty() const { return !data || !data[0]; }
  void clear() { data = NULL; }
};

struct Struct;

template <class T, Size N>
struct Array {
  T at[N];
  T& operator[] (Size index) {
    if (index >= N) throw std::out_of_range("Array index out of range");
    return at[index];
  }
  const T& operator[] (Size index) const {
    if (index >= N) throw std::out_of_range("Array index out of range");
    return at[index];
  }
  bool empty() const { return N == 0; }
  Size size() const { return N; }
  T* begin() { return &at[0]; }
  T* end() { return &at[0] + N; }
  const T* begin() const { return &at[0]; }
  const T* end() const { return &at[0] + N; }
};

template <class T>
struct List {
  typedef T value_type;
  template <class U>
  struct iterator_type {
    const List<U>* ptr;
    iterator_type() : ptr(0) {}
    iterator_type(const List<U>* p) : ptr(p) {}
    iterator_type<U>& operator++ () {
      if (ptr && ptr->next)
        ptr = ptr->next;
      else
        ptr = 0;
      return *this;
    }
    iterator_type<U> operator++ (int) {
      iterator_type<U> copy(*this);
      ++(*this);
      return copy;
    }
    U* operator-> () const {
      return ptr->value.get();
    }
    U& operator* () const {
      return *ptr->value;
    }
    bool operator== (const iterator_type<U>& o) const {
      return ptr == o.ptr;
    }
    bool operator!= (const iterator_type<U>& o) const {
      return !(*this == o);
    }
  };
  typedef iterator_type<T> iterator;
  typedef iterator_type<const T> const_iterator;
  // objects of content_type can be created outside the Struct's memory block
  typedef struct {
    Size size;
    T* value;
    List<T>* next;
  } content_type;

  Size size_;
  OffsetPtr<T> value;
  OffsetPtr<List<T> > next;

  List<T>& operator= (const content_type& a) {
    size_ = a.size;
    value = a.value;
    next = a.next;
    return *this;
  }
  T& operator[] (Size index) {
    if (index >= size_) throw std::out_of_range("List index out of range");
    if (index == 0)
      return *value;
    else
      return (*next)[index - 1];
  }
  const T& operator[] (Size index) const {
    if (index >= size_) throw std::out_of_range("List index out of range");
    if (index == 0)
      return *value;
    else
      return (*next)[index - 1];
  }
  bool operator== (const List<T>& o) const {
    if (size_ != o.size_)
      return false;
    if (!empty() && !o.empty()) {
      for (const_iterator i = begin(), j = o.begin(); i != end(); ++i, ++j) {
        if (!(*i == *j))
          return false;
      }
    }
    return true;
  }
  bool operator!= (const List<T>& o) const { return !(*this == o); }
  void clear() { size_ = 0; value = NULL; next = NULL; }
  bool empty() const { return !size_; }
  Size size() const { return size_; }
  iterator begin() { return empty() ? iterator() : iterator(this); }
  iterator end() { return iterator(); }
  const_iterator begin() const {
    return empty() ? const_iterator() : const_iterator((List<const T>*)this);
  }
  const_iterator end() const { return const_iterator(); }
  template <class V>
  bool append(Struct* allocator, V* v);
};

template <class T>
struct Vector {
  typedef T value_type;
  typedef value_type* iterator;
  typedef const value_type* const_iterator;
  // objects of content_type can be created outside the Struct's memory block
  typedef struct {
    Size size;
    T* at;
  } content_type;

  Size size_;
  OffsetPtr<value_type> at;

  Vector<T>& operator= (const content_type& a) {
    size_ = a.size;
    at = a.at;
    return *this;
  }
  T& operator[] (Size index) {
    if (index >= size_) throw std::out_of_range("Vector index out of range");
    return at[index];
  }
  const T& operator[] (Size index) const {
    if (index >= size_) throw std::out_of_range("Vector index out of range");
    return at[index];
  }
  bool operator== (const Vector<T>& o) const {
    if (size_ != o.size_)
      return false;
    if (!empty() && !o.empty()) {
      for (const_iterator i = begin(), j = o.begin(); i != end(); ++i, ++j) {
        if (!(*i == *j))
          return false;
      }
    }
    return true;
  }
  bool operator!= (const Vector<T>& o) const { return !(*this == o); }
  void clear() { size_ = 0; at = NULL; }
  bool empty() const { return !size_; }
  Size size() const { return size_; }
  iterator begin() { return &at[0]; }
  iterator end() { return &at[0] + size_; }
  const_iterator begin() const { return &at[0]; }
  const_iterator end() const { return &at[0] + size_; }
};

template <class A, class B>
struct Pair {
  A first;
  B second;
};

template <class K, class V>
struct Map : Vector<Pair<K, V> > {
  typedef Vector<Pair<K, V> > base_type;
  typedef typename base_type::value_type value_type;
  typedef value_type* iterator;
  typedef const value_type* const_iterator;
  typedef typename base_type::content_type content_type;
  using base_type::operator=;
  using base_type::operator==;
  using base_type::operator!=;
  using base_type::clear;
  using base_type::empty;
  using base_type::size;
  using base_type::begin;
  using base_type::end;

  V& operator[] (const K& key) {
    iterator it = find(key);
    if (it == end()) throw std::out_of_range("nonexistent key in Map");
    return *it;
  }
  const V& operator[] (const K& key) const {
    const_iterator it = find(key);
    if (it == end()) throw std::out_of_range("nonexistent key in Map");
    return *it;
  }
  iterator find(const K& key) {
    iterator it = begin();
    while (it != end() && !(it->first == key)) ++it;
    return it;
  }
  const_iterator find(const K& key) const {
    const_iterator it = begin();
    while (it != end() && !(it->first == key)) ++it;
    return it;
  }
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

  template <class T, Size N>
  Array<T, N>* CreateArray();

  template <class T>
  const typename List<T>::content_type CreateList(size_t size);

  template <class T>
  const typename Vector<T>::content_type CreateVector(size_t size);

  template <class K, class V>
  const typename Map<K, V>::content_type CreateMap(size_t size);

  const char* CreateString(const std::string &str);
  const wchar_t* CreateWString(const std::wstring &str);
};

// function definitions

template <class T>
template <class V>
bool List<T>::append(Struct* s, V* v) {
  if (!v) return false;
  if (empty()) {
    value = v;
    ++size_;
    return true;
  }
  if (!next) {
    next = s->Allocate<List<T> >();
    if (!next)
      return false;
  }
  if (!next->append(s, v))
    return false;
  size_ = next->size_ + 1;
  return true;
}

template <class S>
S* Struct::New(Size capacity) {
  return InplaceNew<S>(malloc(capacity), capacity);
}

template <class S>
S* Struct::InplaceNew(void* buffer, Size capacity) {
  S* s = new (buffer) S;
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
  T* a = Allocate<T>(count);
  if (a) {
    for (size_t i = 0; i < count; ++i)
      a[i].template Init<T>();
  }
  return a;
}

template <class T, Size N>
Array<T, N>* Struct::CreateArray() {
  Array<T, N>* a = Allocate<Array<T, N> >();
  if (a) {
    for (size_t i = 0; i < a->size(); ++i)
      a->at[i].template Init<T>();
  }
  return a;
}

template <class T>
const typename List<T>::content_type Struct::CreateList(size_t size) {
  typename List<T>::content_type x;
  x.size = size;
  x.value = NULL;
  x.next = NULL;
  if (size > 0) {
    x.value = Allocate<T>();
  }
  if (size > 1) {
    x.next = Allocate<List<T> >();
    *x.next = CreateList<T>(size - 1);
  }
  return x;
}

template <class T>
const typename Vector<T>::content_type Struct::CreateVector(size_t size) {
  typename Vector<T>::content_type x;
  x.size = size;
  x.at = Allocate<T>(size);
  return x;
}

template <class K, class V>
const typename Map<K, V>::content_type Struct::CreateMap(size_t size) {
  return CreateVector<typename Map<K, V>::value_type>(size);
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
