#ifndef DISPATCH_QUEUE_H_
#define DISPATCH_QUEUE_H_

/*
https://embeddedartistry.com/blog/2018/01/29/implementing-an-asynchronous-dispatch-queue-with-freertos/
*/

#include <Arduino.h>
//#include <cassert>
//#include <condition_variable>
#include <cstdint>
#include <functional>
//#include <mutex>
#include <queue>
#include <string>
#include <vector>

/// Definitions for dispatch event flags
#define DISPATCH_WAKE_EVT (0x1)
#define DISPATCH_EXIT_EVT (0x2)

/// Example thread priority and time slice
#define DISPATCH_Q_PRIORITY 15

/// Thread type
struct freertos_thread_t
{
    TaskHandle_t thread;
    std::string name;
};

/// This Bounce implementation is pulled from bounce.cpp
template <class T, class Method, Method m, class... Params>
static auto
bounce (void *priv, Params... params)
    -> decltype (((*reinterpret_cast<T *> (priv)).*m) (params...))
{
    return ((*reinterpret_cast<T *> (priv)).*m) (params...);
}
 
/// Convenience macro to simplify bounce statement usage
#define BOUNCE(c, m) bounce<c, decltype (&c::m), &c::m>

class dispatch_queue
{
    typedef std::function<void (void)> fp_t;

  public:
    dispatch_queue (std::string name, size_t thread_cnt = 1, size_t thread_stack = 1024);
    ~dispatch_queue ();

    // dispatch and copy
    void dispatch (const fp_t &op);
    // dispatch and move
    void dispatch (fp_t &&op);

    // Deleted operations
    dispatch_queue (const dispatch_queue &rhs) = delete;
    dispatch_queue &operator= (const dispatch_queue &rhs) = delete;
    dispatch_queue (dispatch_queue &&rhs) = delete;
    dispatch_queue &operator= (dispatch_queue &&rhs) = delete;

  private:
    std::string name_;

    // FreeRTOS uses semaphore handles for mutexes
    SemaphoreHandle_t mutex_;

    /// Vector of FreeRTOS Threads
    std::vector<freertos_thread_t> threads_;

    /// FreeRTOS event flags - like condition variable, used for waking queue threads
    EventGroupHandle_t notify_flags_;

    std::queue<fp_t> q_;
    bool quit_ = false;

    void dispatch_thread_handler (void);
};

#endif // DISPATCH_QUEUE_H_