---
title: '[1] ARK-Tool develop log : Global Overloaded new & delete.'
date: 2016-02-08 20:02:03
tags: [内核,ARK,Windows,DTL]
---

# 全局重载 New & Delete（DTL系列）

由于在驱动层，WDK并没有提供 new 和 delete，所以我们要用内核的内存分配函数自己重载一套。

new 和 delete 的重载有以下注意的地方：

1. 要符合 C++ 标准
2. 有 new 就要有对应的 delete

以下 new & delete **必须重载**，以供 C++ 基本使用：

1. 一般形式
2. 数组形式
3. placement new
4. 用于类对象的 delete

重载 operator new 的参数个数是可以任意的 , 只需要保证第一个参数为 size_t, 返回类型为 void * 即可 , 而且其重载的参数类型也不必包含自定义类型 . 更一般的说 , operator new 的重载更像是一个函数的重载 , 而不是一个操作符的重载。

<!--more-->

```
// 默认参数值
static const POOL_TYPE DEFAULT_NEW_POOL_TYPE    = NonPagedPool;
static const unsigned long DEFAULT_NEW_TAG      = ' New';

// 一般形式
void * __cdecl operator new (size_t aSize) noexcept
{
    if (0 == aSize)
    {
        // 按照 C++ 标准，
        // 当 size 为 0 时，
        // 申请 1 字节内存
        aSize = 1;
    }
    
    return ExAllocatePoolWithTag(DEFAULT_NEW_POOL_TYPE, aSize, DEFAULT_NEW_TAG);
}
void __cdecl operator delete (void *aPtr) noexcept
{
    if (nullptr == aPtr)
    {
        return;
    }

    return ExFreePoolWithTag(aPtr, DEFAULT_NEW_TAG);
}

// placement new
// 不需要delete，对象可以直接调用析构
void * __cdecl operator new (size_t /*aSize*/, void *aBuffer) noexcept
{
    // placement new的作用就是：创建对象(调用该类的构造函数)但是不分配内存，
    // 而是在已有的内存块上面创建对象。用于需要反复创建并删除的对象上，
    // 可以降低分配释放内存的性能消耗
    return aBuffer;
}

// 数组形式
void * __cdecl operator new[] (size_t aSize) noexcept
{
    if (0 == aSize)
    {
        aSize = 1;
    }

    return ExAllocatePoolWithTag(DEFAULT_NEW_POOL_TYPE, aSize, DEFAULT_NEW_TAG);
}
void __cdecl operator delete[] (void *aPtr) noexcept
{
    if (nullptr == aPtr)
    {
        return;
    }

    return ExFreePoolWithTag(aPtr, DEFAULT_NEW_TAG);
}

// 用于类对象的 delete
void __cdecl operator delete (void *aPtr, size_t /*aSize*/) noexcept
{
    // sized class - specific deallocation functions
    if (nullptr == aPtr)
    {
        return;
    }

    return ExFreePoolWithTag(aPtr, DEFAULT_NEW_TAG);
}
void __cdecl operator delete[] (void *aPtr, size_t /*aSize*/) noexcept
{
    // sized class - specific deallocation functions
    if (nullptr == aPtr)
    {
        return;
    }

    return ExFreePoolWithTag(aPtr, DEFAULT_NEW_TAG);
}
```

