#include <iostream>
#include "mighty_struct.h"

using namespace std;

struct Point2D : Struct {
  int x;
  int y;
};

struct Point3D : Point2D {
  int z;

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
  here.Init<Point2D>();
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

  void Print() const;
};

void Student::Print() const {
  cout << "[capacity]: " << capacity << endl;
  cout << "[used_space]: " << used_space << endl;
  cout << "name: " << name.c_str() << endl;
  cout << "age: " << age << endl;
  cout << "courses:";
  for (size_t i = 0; i < courses.size(); ++i) {
    cout << ' ' << courses[i].c_str();
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
}

void test_student() {
  Student *s = Struct::New<Student>(512);
  s->name = s->CreateString("Fred");
  s->age = 20;
  s->courses = s->CreateArray<String>(3);
  s->courses[0] = s->CreateString("chinese");
  s->courses[1] = s->CreateString("english");
  s->courses[2] = s->CreateString("math");
  // creating a nested struct
  s->position = s->Create<Point>();
  s->position->x = 3;
  s->position->y = 2;
  s->position->z = 1;
  s->scores = s->CreateMap<String, int>(s->courses.size());
  Vector<String>::const_iterator i = s->courses.begin();
  Map<String, int>::iterator j = s->scores.begin();
  int score = 90;
  for (; i != s->courses.end(); ++i, ++j) {
    j->first = *i;
    j->second = score++;
  }
  // done
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
