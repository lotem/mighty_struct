# MightyStruct Explained

GONG Chen <chen.sst@gmail.com>

2013-03-24

## Problem 1

Assume we have designed the following program with a plugin.

```
// point_v1.h
struct Point {
  int x;
  int y;
};

// in module main_v1.cc
extern void move_point(Point* p);
int main() {
  Point p;
  p.x = 1;
  p.y = -1;
  move_point(&p);
  //use(p);
  return 0;
}

// in module plugin_v1.cc
void move_point(Point* p) {
  --p->x;
  ++p->y;
}
```

In the next version, we decide to go 3D.

```
// point_v2.h
struct Point {
  int x;
  int y;
  // since v2
  int z;
};

// in module plugin_v2.cc
void move_point(Point* p) {
  --p->x;
  ++p->y;
  // what if the point comes from main_v1 ?
  p->z += 2;
}
```

It would definitely be cool for the plugin to be compatible with
older versions of the program.

```
// point_v2.h with manually versioned struct
struct Point {
  int version;
  int x;
  int y;
  // since v2
  int z;
};

// in module main_v2.cc
extern void move_point(Point* p);
int main() {
  Point p;
  p.version = 2;
  p.x = 1;
  p.y = -1;
  p.z = 0;
  move_point(&p);
  //use(p);
  return 0;
}

// in module plugin_v2.cc
void move_point(Point* p) {
  --p->x;
  ++p->y;
  if (p->version >= 2)
    p->z += 2;
}
```

Problem solved, but frequently you have to refer to the struct's definition for versions in which members were added. That's not cool.

## Solution to Problem 1

```
// point_v1.h powered by MightyStruct
#include "mighty_struct.h"
using namespace mighty;

struct Point : Struct {
  int x;
  int y;

  Point() : x(0), y(0) {
    Init<Point>();
  }
};
```

`mighty::Struct` must be the first base class of a user defined MightyStruct type.

By initializing data member `struct_size` by the call to `Init<T>()`, `mighty::Struct` handles versioning automatically:

```
// point_v2.h
#include "mighty_struct.h"
using namespace mighty;

struct Point : Struct {
  int x;
  int y;
  int z;  // simply grow the struct

  Point() : x(0), y(0), z(0) {
    Init<Point>();
  }

  void Print() const;
};
```

Let's see how to use `Point` objects.

```
// point_v2.cc
#include <iostream>
void Point::Print() const {
  std::cout << x << ", " << y;
  if (HasMember(z))
    std::cout << ", " << z;
}

// in module main_v2.cc
extern void move_point(Point* p);
int main() {
  Point p(1, -1, 0);
  move_point(&p);
  //use(p);
  return 0;
}

// in module plugin_v2.cc
void move_point(Point* p) {
  --p->x;
  ++p->y;
  if (p->HasMember(p->z))
    p->z += 2;
}
```

Forget about the version; just check for availability of the data member you need access to.

# Problem 2

```
// student_v1.h

const int MAX_LEN = 50;
const int MAX_COURSES = 20;
const int MAX_CLASSMATES = 100;

struct Student {
  char name[MAX_LEN];
  int age;
  char courses[MAX_COURSES][MAX_LEN];
  // planning to go social in the next version
  //Student* classmates[MAX_CLASSMATES];
};
```

By using fixed length arrays in the struct, we

  - waste memory to handle worst cases
  - set limits that hinder scalability

That means bad API design.

```
// student_v2.h

struct Student {
  char *name;
  int age;
  int num_courses;
  char** courses;
  // planning to go social in the next version
  //int num_classmates;
  //Student** classmates;
};


// main_v2.cc
#include <student_v2.h>

int main() {
  Student fred;
  fred.name = new char[strlen("Fred") + 1];
  strcpy(fred.name, "Fred");
  fred.age = 20;
  fred.num_courses = 3;
  fred.courses = new char*[fred.num_courses];
  fred.courses[0] = new char[strlen("chinese") + 1];
  strcpy(fred.courses[0], "chinese");
  // ...
  // salute(fred);
  // ...
  // when we are finished
  delete[] fred.name;
  for (int i = 0; i < fred.num_courses; ++i) {
    delete[] fred.courses[i];
  }
  delete[] fred.courses;
  // ...
  return 0;
}

```

Manual memory management is tiresome and error-prone.
Well, may I present you MightyStruct again.

# Solution to Problem 2

```
// student_v3.h
#include "mighty_struct.h"
using namespace mighty;

struct Student : Struct {
  String name;
  int age;
  Vector<String> courses;

  Student() : age(0) {
    Init<Student>();
  }

  void Print() const;
};

// student_v3.cc

void Student::Print() const {
  cout << "[struct_size]: " << struct_size << endl;
  cout << "[capacity]: " << capacity() << endl;
  cout << "[used_space]: " << used_space() << endl;

  cout << "name: " << name.c_str() << endl;
  cout << "age: " << age << endl;
  cout << "courses: ";
  for (size_t i = 0; i < courses.size(); ++i) {
    if (i != 0) cout << ", ";
    cout << courses[i].c_str();
  }
  cout << endl;
}
```

MightyStruct provides STL-look, variable length containers.
The variable length contents live in pre-allocated free space right after the struct.

```
// main_v3.cc

int main() {
  // allocate a Student object with 300 bytes of free space
  Student *s = Struct::New<Student>(300);
  s->name = s->CreateString("Fred");
  s->age = 20;
  // create a Vector
  s->courses = s->CreateVector<String>(3);
  if (!s->courses.empty()) {
    s->courses[0] = s->CreateString("chinese");
    s->courses[1] = s->CreateString("english");
    s->courses[2] = s->CreateString("math");
  }

  s->Print();

  // allocate space for the replica to fit the size of the populated original copy
  Student *t = Struct::NewCopy(s);
  t->Print();
  // there's no more space in the copied Struct for new contents
  // new allocations will fail

  char buffer[1000];
  Student *r = Struct::InplaceNew<Student>(buffer, sizeof(buffer));
  r->Copy(s);
  r->name = r->CreateString(std::string("Poor ") + r->name.c_str());
  r->Print();

  Struct::Delete(t);
  Struct::Delete(s);

  return 0;
}
```

Let's go social and upgrade the Sturct.

```
// student_v4.h
#include "mighty_struct.h"
using namespace mighty;

struct Student : Struct {
  String name;
  int age;
  Vector<String> courses;
  // since v4
  List<Student> classmates;

  Student() : age(0) {
    Init<Student>();
  }

  void Print() const;
};

// student_v4.cc

void Student::Print() const {
  cout << "name: " << name.c_str() << endl;
  cout << "age: " << age << endl;
  // ...
  if (HasMember(classmates) && !classmates.empty()) {
    cout << "classmates: ";
    for (List<Student>::const_iterator i = classmates.begin(); i != classmates.end(); ++i) {
      if (i != classmates.begin()) cout << ", ";
      cout << i->name.c_str();
    }
    cout << endl;
  }
}

// main_v4.cc

int main() {
  Student *s = Struct::New<Student>(1000);
  s->name = s->CreateString("Fred");
  s->age = 20;
  // make some friends
  Student* lilei = s->Create<Student>();
  if (lilei) {
    lilei->name = s->CreateString("Li Lei");
    s->classmates->append(s, lilei);
  }
  Student* hanmeimei = s->Create<Student>();
  if (hanmeimei) {
    hanmeimei->name = s->CreateString("Han Meimei");
    s->classmates->append(s, hanmeimei);
  }
  s->Print();
  Struct::Delete(s);
  return 0;
}

```

# The Ultimate Solution

Instead of creating Structs with `Struct::New` or by calling `Struct::InplaceNew` on a buffer, we can create objects directly on the stack:

```
  Mighty<Student, 1024> fred;

```

which is equivalent to:

```
  struct MightyStudent : Student, FreeSpace<1024 - sizeof(Student)> {
    MightyStudent() {
      Init<Student>(1024 - sizeof(Student));
    }
  } fred;
```
