#pragma once

/**
 * Blocking bound queue.
 * 
 * 
*/

#include <mutex>
#include <condition_variable>
#include <queue>

namespace wshutil{

static const int _FULL = 128;

template <typename _Tp>
class BlockBoundQueue
{
public:
    BlockBoundQueue(const BlockBoundQueue &) = delete;
    BlockBoundQueue & operator=(const BlockBoundQueue &) = delete;

    BlockBoundQueue(size_t bound = _FULL) :bound(bound) {}

    void push(const _Tp & value) {
        std::unique_lock<std::mutex> lck(mutex_);// 利用RAII技法，将mutex_托管给lck
        while (count + 1 == bound) {
            //printf("the queue is full , waiting for the consumer consuming !\n");
            notFull_.wait(lck); //等待队列非满条件发生
        }
        count++;
        queue.push(value);
        notEmpty_.notify_one();//通知队列非空，不能用all，读者自行思考为什么
    }

    _Tp pop() {
        std::unique_lock<std::mutex> lck(mutex_);
        while (queue.empty()) {
            //printf("the queue is empty , waiting for the producer producing !\n");
            notEmpty_.wait(lck);//等待队列为非空
        }
        _Tp front(queue.front());
        queue.pop();
        count--;
        notFull_.notify_one();//通知队列为非满，请注意不能用all
        return front;
    }
private:
    std::mutex mutex_;
    std::condition_variable notEmpty_;
    std::condition_variable notFull_;
    std::queue<_Tp> queue;
    size_t count;
    size_t bound;
};
} // end of namespace

