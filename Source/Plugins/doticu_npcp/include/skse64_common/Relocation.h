#pragma once

#include <cstdint>

class RelocationManager
{
public:
    static uintptr_t s_baseAddr;

    static bool Initialize();
    static bool HasDatabase();
    static uintptr_t Resolve(uint64_t id, uintptr_t fallbackOffset);
};

template <class T>
class RelocPtr
{
public:
    explicit RelocPtr(uintptr_t offset = 0) :
        _offset(offset)
    {
    }

    uintptr_t GetUIntPtr() const
    {
        return RelocationManager::s_baseAddr + _offset;
    }

    T* get() const
    {
        return reinterpret_cast<T*>(GetUIntPtr());
    }

    operator T*() const
    {
        return get();
    }

    T* operator->() const
    {
        return get();
    }

    T& operator*() const
    {
        return *get();
    }

private:
    uintptr_t _offset;
};

template <class T>
class RelocAddr
{
public:
    explicit RelocAddr(uintptr_t offset = 0) :
        _offset(offset)
    {
    }

    uintptr_t GetUIntPtr() const
    {
        return RelocationManager::s_baseAddr + _offset;
    }

    operator T() const
    {
        return (T)(GetUIntPtr());
    }

private:
    uintptr_t _offset;
};
