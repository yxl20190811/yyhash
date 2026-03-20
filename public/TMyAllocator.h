#ifndef __MyAllocator_H__
#define __MyAllocator_H__
#pragma once
#include <memory>
#include <cstddef>
#include <new>
#include <iostream>

template <typename T>
class MyAllocator
{
public:
    using value_type = T;

    // �����ṩĬ�Ϲ���
    MyAllocator() noexcept {}

    // �����ṩģ�忽������
    template <class U>
    MyAllocator(const MyAllocator<U>&) noexcept {}

    // �����ڴ�
    T* allocate(std::size_t n)
    {
        if (n > std::size_t(-1) / sizeof(T))
            throw std::bad_alloc();

        void* p = std::malloc(n * sizeof(T));
        if (!p)
            throw std::bad_alloc();

        //std::cout << "[allocate] " << n << " objects\n";
        return static_cast<T*>(p);
    }

    // �ͷ��ڴ�
    void deallocate(T* p, std::size_t n) noexcept
    {
        //std::cout << "[deallocate] " << n << " objects\n";
        std::free(p);
    }

    // C++17 �Ժ��ٱ��룬��Ϊ�˼��� VC ����
    template <class U>
    struct rebind
    {
        using other = MyAllocator<U>;
    };
};

// �����ṩ�Ƚ������
template <class T, class U>
bool operator==(const MyAllocator<T>&, const MyAllocator<U>&)
{
    return true;
}

template <class T, class U>
bool operator!=(const MyAllocator<T>&, const MyAllocator<U>&)
{
    return false;
}
#endif //__MyAllocator_H__
