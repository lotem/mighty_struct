#include <iostream>
#include "mighty_struct.h"

using namespace std;
using namespace mighty;

struct Point2D : Struct {
  int x;
  int y;

  Point2D() : x(0), y(0) {
    Init<Point2D>();
  }
};

struct Point3D : Point2D {
  int z;

  Point3D() : z(0) {
    Init<Point3D>();
  }

  void Print() const;
};

void Point3D::Print() const {
  cout << x << ", " << y;
  if (HasMember( z )) {
    cout << ", " << z;
  }
}

// current version
typedef Point3D Point;

void test_point() {
  Point2D here;
  here.x = 1;
  here.y = 2;

  // using it elsewhere
  Point* p = (Point*)(&here);
  cout << "there: ";
  p->Print();
  cout << endl;
}

struct Student : Struct {
  String name;
  int age;
  Vector<String> courses;
  // extensible structure should be intergrated by pointer
  OffsetPtr<Point> position;
  // so that Student can be extended further more
  Map<String, int> scores;

  // use Vector<> for fixed length types such as int, String
  // use List<> for variable length (extensible) Structs
  List<Student> classmates;

  Student() : age(0) {
    Init<Student>();
  }

  void Print() const;
};

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
    if (HasMember(scores) && !scores.empty()) {
      Map<String, int>::const_iterator score = scores.find(courses[i]);
      if (score != scores.end()) {
        cout << '(' << score->second << ')';
      }
    }
  }
  cout << endl;
  if (position) {
    cout << "position: ";
    position->Print();
    cout << endl;
  }
  if (HasMember(classmates) && !classmates.empty()) {
    cout << "classmates: ";
    for (List<Student>::const_iterator i = classmates.begin(); i != classmates.end(); ++i) {
      if (i != classmates.begin()) cout << ", ";
      cout << i->name.c_str();
    }
    cout << endl;
  }
}

void test_student() {
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
  // create a nested Struct
  s->position = s->Create<Point>();
  if (s->position) {
    s->position->x = 3;
    s->position->y = 2;
    s->position->z = 1;
  }
  // create a Map
  s->scores = s->CreateMap<String, int>(s->courses.size());
  Vector<String>::const_iterator i = s->courses.begin();
  Map<String, int>::iterator j = s->scores.begin();
  int score = 90;
  for (; i != s->courses.end(); ++i, ++j) {
    j->first = *i;
    j->second = score++;
  }
  // create a linked List of Student
  s->classmates = s->CreateList<Student>(2);
  if (!s->classmates.empty()) {
    s->classmates[0].name = s->CreateString("Li Lei");
    s->classmates[1].name = s->CreateString("Han Meimei");
  }
  // appending a Student to the list
  Student* new_classmate = s->Create<Student>();
  // check if the class has run out of room for the new comer
  if (new_classmate) {
    new_classmate->name = s->CreateString("Jim Green");
    s->classmates.append(s, new_classmate);
  }
  // done
  cout << "# s" << endl;
  s->Print();

  Student *t = Struct::NewCopy(s);
  cout << "# t" << endl;
  t->Print();

  char buffer[1000];
  Student *r = Struct::InplaceNew<Student>(buffer, sizeof(buffer));
  r->Copy(s);
  r->classmates.clear();
  r->name = r->CreateString(std::string("Lonely ") + r->name.c_str());
  cout << "# r" << endl;
  r->Print();

  struct UltraStudent : Student, FreeSpace<500> {
    UltraStudent() {
      Init<Student>(500);
    }
  } u;
  u.Copy(s);
  cout << "# u" << endl;
  u.Print();

  Mighty<Student, 1024> v;
  v.Copy(s);
  cout << "# v" << endl;
  v.Print();

  Struct::Delete(t);
  Struct::Delete(s);
}

int main() {
  test_point();
  test_student();
  return 0;
}
