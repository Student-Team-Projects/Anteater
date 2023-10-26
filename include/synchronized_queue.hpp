#pragma once

#include <atomic>
#include <mutex>
#include <cstring>

// synchronized queue for 1 reader and 1 writer
template <typename T>
class synchronized_queue {
    T *data;
    size_t cap, head, tail;
    std::atomic_size_t unread;
    std::mutex resize_mu;
    std::atomic_bool unblocked;

  public:
    explicit synchronized_queue(size_t cap) : data(new T[cap]), cap(cap), head(0), tail(0), unread(0) {}

    void push(T &&val) {
        if (unread == cap) [[unlikely]] {
            // lock to block potential pop(), resize even if unread falls below cap
            std::lock_guard<std::mutex> l(resize_mu);
            T *old_data = data;
            data = new T[2*cap];
            
            // move current data to front of buffer
            memmove(data, old_data + tail, sizeof(T) * (cap - tail));
            memmove(data + cap-tail, old_data, sizeof(T) * head);

            // deallocate old underlying array
            delete[] old_data;
            
            // update head and tail
            head += cap - tail;
            tail = 0;

            // update capacity
            cap *= 2;
        }

        data[head++] = std::move(val);
        ++unread;
        if (unread == 1)
            unread.notify_one();
        if (head == cap) [[unlikely]] {
            // circular buffer
            head -= cap;
        }
    }

    T pop() {
        if (unread == 0) unread.wait(0, std::memory_order_relaxed);
        if (unblocked) [[unlikely]] return T();
        std::lock_guard<std::mutex> l(resize_mu);

        T res = std::move(data[tail++]);
        if (tail == cap) [[unlikely]] {
            // circular buffer
            tail -= cap;
        }
        --unread;
        return res;
    }

    void unblock() {
        unblocked = true;
        unread = 1;
        unread.notify_all();
    }

    ~synchronized_queue() {
        delete[] data;
    };
};
