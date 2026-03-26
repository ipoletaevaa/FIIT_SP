#include "allocator/allocator_global_heap/allocator_global_heap.h"
#include <iostream>
#include <thread>
#include <vector>

class TestClass {
private:
    int value;
public:
    TestClass(int v) : value(v) {}
    int getValue() const { return value; }
};

void thread_work(AllocatorGlobalHeap& alloc, int id) {
    int* i = static_cast<int*>(alloc.allocate(sizeof(int)));
    *i = id;
    
    double* d = static_cast<double*>(alloc.allocate(sizeof(double)));
    *d = id * 1.5;
    
    TestClass* obj = static_cast<TestClass*>(alloc.allocate(sizeof(TestClass)));
    new (obj) TestClass(id);
    
    std::cout << "Thread " << id << ": int=" << *i 
              << ", double=" << *d 
              << ", obj=" << obj->getValue() << std::endl;
    
    obj->~TestClass();
    alloc.deallocate(obj);
    alloc.deallocate(d);
    alloc.deallocate(i);
}

int main() {
    try {
        AllocatorGlobalHeap allocator;
        
        int* pi = static_cast<int*>(allocator.allocate(sizeof(int)));
        *pi = 42;
        std::cout << "int: " << *pi << std::endl;
        allocator.deallocate(pi);
        
        double* pd = static_cast<double*>(allocator.allocate(sizeof(double)));
        *pd = 3.14;
        std::cout << "double: " << *pd << std::endl;
        allocator.deallocate(pd);
        
        TestClass* obj = static_cast<TestClass*>(allocator.allocate(sizeof(TestClass)));
        new (obj) TestClass(100);
        std::cout << "object value: " << obj->getValue() << std::endl;
        obj->~TestClass();
        allocator.deallocate(obj);
        
        std::vector<std::thread> threads;
        for (int i = 0; i < 3; ++i) {
            threads.emplace_back(thread_work, std::ref(allocator), i);
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}