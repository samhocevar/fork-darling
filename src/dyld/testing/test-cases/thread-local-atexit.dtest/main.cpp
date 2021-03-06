
// BUILD:  $CXX main.cpp -std=c++11 -o $BUILD_DIR/thread-local-atexit.exe

// RUN:  ./thread-local-atexit.exe

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// We create an A and a B.
// While destroying B we create a C
// Given that tlv_finalize has "destroy in reverse order of construction", we
// must then immediately destroy C before we destroy A to maintain that invariant

enum State {
    None,
    ConstructedA,
    ConstructedB,
    ConstructedC,
    DestroyingB,
    DestroyedA,
    DestroyedB,
    DestroyedC,
};

struct A {
    A();
    ~A();
};

struct B {
    B();
    ~B();
};

struct C {
    C();
    ~C();
};

State state;

A::A() {
    if ( state != None ) {
        printf("[FAIL] thread-local-atexit: should be in the 'None' state\n");
    }
    state = ConstructedA;
}

B::B() {
    if ( state != ConstructedA ) {
        printf("[FAIL] thread-local-atexit: should be in the 'ConstructedA' state\n");
        _Exit(0);
    }
    state = ConstructedB;
}

C::C() {
    // We construct C during B's destructor
    if ( state != DestroyingB ) {
        printf("[FAIL] thread-local-atexit: should be in the 'DestroyingB' state\n");
        _Exit(0);
    }
    state = ConstructedC;
}

// We destroy B first
B::~B() {
    if ( state != ConstructedB ) {
        printf("[FAIL] thread-local-atexit: should be in the 'ConstructedB' state\n");
        _Exit(0);
    }
    state = DestroyingB;
    static thread_local C c;
    if ( state != ConstructedC ) {
        printf("[FAIL] thread-local-atexit: should be in the 'ConstructedC' state\n");
        _Exit(0);
    }
    state = DestroyedB;
}

// Then we destroy C
C::~C() {
    if ( state != DestroyedB ) {
        printf("[FAIL] thread-local-atexit: should be in the 'DestroyedB' state\n");
        _Exit(0);
    }
    state = DestroyedC;
}

// And finally destroy A
A::~A() {
    if ( state != DestroyedC ) {
        printf("[FAIL] thread-local-atexit: should be in the 'DestroyedC' state\n");
        _Exit(0);
    }
    state = DestroyedA;
    printf("[PASS] thread-local-atexit\n");
}

static void* work(void* arg)
{
    thread_local A a;
    thread_local B b;

    return NULL;
}

int main() {
    printf("[BEGIN] thread-local-atexit\n");

    pthread_t worker;
    if ( pthread_create(&worker, NULL, work, NULL) != 0 ) {
        printf("[FAIL] thread-local-atexit, pthread_create\n");
        return 0;
    }

    void* dummy;
    pthread_join(worker, &dummy);

    return 0;
}

