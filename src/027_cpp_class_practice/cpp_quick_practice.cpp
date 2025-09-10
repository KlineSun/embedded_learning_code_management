#include <iostream>
#include <stdio.h>

using namespace std;

class Acls {
public:
    Acls() {};
    ~Acls() {};
};

class Bcls {
public:
    Bcls() {};
    virtual ~Bcls() {};

    virtual void sayHello() {
        std::cout << "Helloc in class Bcls" << endl;
    };
};

class Ccls : public Acls, public Bcls {
public:

    Ccls() : member2(1) {};
    virtual ~Ccls() {};

    virtual void sayHello() {
        std::cout << "Helloc in class Ccls" << endl;
    };

    inline static int member1 = 10;
    int member2;
};

// int Ccls::member1 = 1;


int main(int argc, const char **argv)
{
    int lenA = 0, lenB= 0, lenC = 0;

    lenA = sizeof(Acls);
    lenB = sizeof(Bcls);
    lenC = sizeof(Ccls);

    printf("lenA = %d, lenB=%d, lenC=%d\n", lenA, lenB, lenC);

    Ccls obj_a;
    Ccls obj_b;

    printf("Before member1 = %d\n", obj_b.member1);
    obj_a.member1++;
    printf("After member1 = %d\n", obj_b.member1);
    obj_a.member1++;
    printf("Finally member1 = %d\n", obj_b.member1);
    

    return 0;
}