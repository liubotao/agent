//
// Created by liubotao on 15/9/30.
//

#ifndef AGENT_MUTEXLOCK_H
#define AGENT_MUTEXLOCK_H

#include <pthread.h>

class MutexLock {

public:
    MutexLock() {pthread_mutex_init(&mutex_, NULL);}

    ~MutexLock(){pthread_mutex_destroy(&mutex_);}

    void lock() { pthread_mutex_lock(&mutex_);}

    void unlock() { pthread_mutex_unlock(&mutex_);}
private:
    pthread_mutex_t mutex_;

private:
    MutexLock(const MutexLock&);
    MutexLock&operator=(const MutexLock&);
};

class MutexLockGuard {
public:
    explicit MutexLockGuard(MutexLock& mutex)
            : mutex_(mutex) {
        mutex_.lock();
    }

    ~MutexLockGuard() {
        mutex_.unlock();
    }

private:
    MutexLock& mutex_;

private:
    MutexLockGuard(const MutexLockGuard&);
    MutexLockGuard&operator=(const MutexLockGuard&);

};
#endif //AGENT_MUTEXLOCK_H
