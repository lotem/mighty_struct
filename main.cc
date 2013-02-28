#include <iostream>
#include "mighty_struct.h"

using namespace std;

struct Point2D : Struct {
  int x;
  int y;
};

struct Point3D : Point2D {
  int z;
};

// current version
typedef Point3D Point;

void test_point() {
  Point2D here;
  here.Init<Point2D>(); 
  here.x = 1;
  here.y = 2;

  // using it elsewhere
  Point* p = (Point*)(&here);
  cout << "there: " << p->x << ", " << p->y;
  if (p->HasMember( p->z )) {
    cout << ", " << p->z;
  }
  cout << endl;
}

struct Student : Struct {
  String name;
  int age;
  List<String> courses;

  void Print() const;
};

void Student::Print() const {
  cout << "[capacity]: " << capacity << endl;
  cout << "[used_space]: " << used_space << endl;
  cout << "name: " << name.c_str() << endl;
  cout << "age: " << age << endl;
  cout << "courses:";
  for (size_t i = 0; i < courses.size; ++i) {
    cout << ' ' << courses.at[i].c_str();
  }
  cout << endl;
}

void test_student() {
  Student *s = Struct::New<Student>(512);
  s->name = s->CreateString("Fred");
  s->age = 20;
  s->courses = s->CreateList<String>(3);
  s->courses.at[0] = s->CreateString("chinese");
  s->courses.at[1] = s->CreateString("english");
  s->courses.at[2] = s->CreateString("math");
  cout << "# s" << endl;
  s->Print();

  Student *t = Struct::NewCopy(s);
  cout << "# t" << endl;
  t->Print();

  char buffer[200];
  Student *r = Struct::InplaceNew<Student>(buffer, sizeof(buffer));
  Struct::Copy(r, s);
  cout << "# r" << endl;
  r->Print();

  Struct::Delete(t);
  Struct::Delete(s);
}

int main() {
  test_point();
  test_student();
  return 0;
}
