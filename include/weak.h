#pragma once

#include "sw_fwd.h"  // Forward declaration
#include "shared.h"

// https://en.cppreference.com/w/cpp/memory/weak_ptr
template<typename T>
class WeakPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    WeakPtr() : ptr_(nullptr), data_(nullptr) {
    }

    WeakPtr(const WeakPtr &other) : ptr_(other.GetPtr()), data_(other.GetData()) {
        if (GetData()) {
            GetData()->WeakConnect();
        }
    }

    template<typename Y>
    WeakPtr(const WeakPtr<Y> &other) : ptr_(other.GetPtr()), data_(other.GetData()) {
        if (GetData()) {
            GetData()->WeakConnect();
        }
    }

    template<typename Y>
    WeakPtr(WeakPtr<Y> &&other) : ptr_(other.GetPtr()), data_(other.GetData()) {
        other.ptr_ = nullptr;
        other.data_ = nullptr;
    }

    template<typename Y>
    WeakPtr(const SharedPtr<Y> &other) : ptr_(other.GetPtr()), data_(other.GetData()) {
        if (GetData()) {
            GetData()->WeakConnect();
        }
    }

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    WeakPtr(const SharedPtr<T> &other) : ptr_(other.GetPtr()), data_(other.GetData()) {
        if (GetData()) {
            GetData()->WeakConnect();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    WeakPtr &operator=(const WeakPtr &other) {
        if (this != &other) {
            Reset();
            GetPtr() = other.GetPtr();
            GetData() = other.GetData();
            if (GetData()) {
                GetData()->WeakConnect();
            }
        }
        return *this;
    }

    WeakPtr &operator=(WeakPtr &&other) {
        Reset();
        std::swap(this->GetData(), other.GetData());
        std::swap(this->GetPtr(), other.GetPtr());
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~WeakPtr() {
        Reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (GetData()) {
            GetPtr() = nullptr;
            GetData()->WeakDisconnect();
            GetData() = nullptr;
        }
    }

    void Swap(WeakPtr &other) {
        std::swap(ptr_, other.ptr_);
        std::swap(data_, other.data_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const noexcept {
        if (GetData() == nullptr) {
            return 0;
        }
        return GetData()->SharedCount();
    }

    bool Expired() const noexcept {
        return UseCount() == 0;
    }

    SharedPtr<T> Lock() const noexcept {
        if (!data_ || !data_->TrySharedConnect()) {
            return SharedPtr<T>();
        }
        return SharedPtr<T>(ptr_, data_);
    }

private:
    T *ptr_;
    ptr_detail::ControlBlock *data_;

    T *&GetPtr() {
        return ptr_;
    }

    ptr_detail::ControlBlock *&GetData() {
        return data_;
    }

    T *GetPtr() const {
        return ptr_;
    }

    ptr_detail::ControlBlock *GetData() const {
        return data_;
    }

    template<typename U>
    friend class WeakPtr;
};
