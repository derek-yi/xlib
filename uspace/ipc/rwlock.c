
//https://www.dazhuanlan.com/2019/10/04/5d970e9b6d162/

//https://man7.org/linux/man-pages/man3/pthread_rwlock_rdlock.3p.html

#include <pthread.h>
#include <time.h>

int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);
int pthread_rwlock_init(pthread_rwlock_t *restrict rwlock, const pthread_rwlockattr_t *restrict attr);

int pthread_rwlock_timedwrlock(pthread_rwlock_t *restrict rwlock, const struct timespec *restrict abstime);
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);


pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

int main()
{
}


