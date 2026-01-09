

#if 1
int   pthread_attr_destroy(pthread_attr_t *);
int   pthread_attr_getdetachstate(const pthread_attr_t *, int *);
int   pthread_attr_getguardsize(const pthread_attr_t *restrict,
         size_t *restrict);
int   pthread_attr_getinheritsched(const pthread_attr_t *restrict,
         int *restrict);
int   pthread_attr_getschedparam(const pthread_attr_t *restrict,
         struct sched_param *restrict);
int   pthread_attr_getschedpolicy(const pthread_attr_t *restrict,
         int *restrict);
int   pthread_attr_getscope(const pthread_attr_t *restrict,
         int *restrict);
int   pthread_attr_getstack(const pthread_attr_t *restrict,
         void **restrict, size_t *restrict);
int   pthread_attr_getstacksize(const pthread_attr_t *restrict,
         size_t *restrict);
int   pthread_attr_init(pthread_attr_t *);
int   pthread_attr_setdetachstate(pthread_attr_t *, int);
int   pthread_attr_setguardsize(pthread_attr_t *, size_t);
int   pthread_attr_setinheritsched(pthread_attr_t *, int);
int   pthread_attr_setschedparam(pthread_attr_t *restrict,
         const struct sched_param *restrict);
int   pthread_attr_setschedpolicy(pthread_attr_t *, int);
int   pthread_attr_setscope(pthread_attr_t *, int);
int   pthread_attr_setstack(pthread_attr_t *, void *, size_t);
int   pthread_attr_setstacksize(pthread_attr_t *, size_t);
#endif

#if 1
int   pthread_barrier_destroy(pthread_barrier_t *);
int   pthread_barrier_init(pthread_barrier_t *restrict,
         const pthread_barrierattr_t *restrict, unsigned);
int   pthread_barrier_wait(pthread_barrier_t *);
int   pthread_barrierattr_destroy(pthread_barrierattr_t *);
int   pthread_barrierattr_getpshared(
         const pthread_barrierattr_t *restrict, int *restrict);
int   pthread_barrierattr_init(pthread_barrierattr_t *);
int   pthread_barrierattr_setpshared(pthread_barrierattr_t *, int);
#endif
         
#if 1

int   pthread_cond_broadcast(pthread_cond_t *);
int   pthread_cond_destroy(pthread_cond_t *);
int   pthread_cond_init(pthread_cond_t *restrict,
         const pthread_condattr_t *restrict);
int   pthread_cond_signal(pthread_cond_t *);
int   pthread_cond_timedwait(pthread_cond_t *restrict,
         pthread_mutex_t *restrict, const struct timespec *restrict);
int   pthread_cond_wait(pthread_cond_t *restrict,
         pthread_mutex_t *restrict);


int   pthread_condattr_destroy(pthread_condattr_t *);
int   pthread_condattr_getclock(const pthread_condattr_t *restrict,
         clockid_t *restrict);
int   pthread_condattr_getpshared(const pthread_condattr_t *restrict,
         int *restrict);
int   pthread_condattr_init(pthread_condattr_t *);
int   pthread_condattr_setclock(pthread_condattr_t *, clockid_t);
int   pthread_condattr_setpshared(pthread_condattr_t *, int);

#endif

#if 1


int   pthread_mutex_consistent(pthread_mutex_t *);
int   pthread_mutex_destroy(pthread_mutex_t *);
int   pthread_mutex_getprioceiling(const pthread_mutex_t *restrict,
         int *restrict);
int   pthread_mutex_init(pthread_mutex_t *restrict,
         const pthread_mutexattr_t *restrict);
int   pthread_mutex_lock(pthread_mutex_t *);
int   pthread_mutex_setprioceiling(pthread_mutex_t *restrict, int,
         int *restrict);
int   pthread_mutex_timedlock(pthread_mutex_t *restrict,
         const struct timespec *restrict);
int   pthread_mutex_trylock(pthread_mutex_t *);
int   pthread_mutex_unlock(pthread_mutex_t *);


int   pthread_mutexattr_destroy(pthread_mutexattr_t *);
int   pthread_mutexattr_getprioceiling(
         const pthread_mutexattr_t *restrict, int *restrict);
int   pthread_mutexattr_getprotocol(const pthread_mutexattr_t *restrict,
         int *restrict);
int   pthread_mutexattr_getpshared(const pthread_mutexattr_t *restrict,
         int *restrict);
int   pthread_mutexattr_getrobust(const pthread_mutexattr_t *restrict,
         int *restrict);
int   pthread_mutexattr_gettype(const pthread_mutexattr_t *restrict,
         int *restrict);
int   pthread_mutexattr_init(pthread_mutexattr_t *);
int   pthread_mutexattr_setprioceiling(pthread_mutexattr_t *, int);
int   pthread_mutexattr_setprotocol(pthread_mutexattr_t *, int);
int   pthread_mutexattr_setpshared(pthread_mutexattr_t *, int);
int   pthread_mutexattr_setrobust(pthread_mutexattr_t *, int);
int   pthread_mutexattr_settype(pthread_mutexattr_t *, int);


#endif

#if 1

int   pthread_rwlock_destroy(pthread_rwlock_t *);
int   pthread_rwlock_init(pthread_rwlock_t *restrict,
         const pthread_rwlockattr_t *restrict);
int   pthread_rwlock_rdlock(pthread_rwlock_t *);
int   pthread_rwlock_timedrdlock(pthread_rwlock_t *restrict,
         const struct timespec *restrict);
int   pthread_rwlock_timedwrlock(pthread_rwlock_t *restrict,
         const struct timespec *restrict);
int   pthread_rwlock_tryrdlock(pthread_rwlock_t *);
int   pthread_rwlock_trywrlock(pthread_rwlock_t *);
int   pthread_rwlock_unlock(pthread_rwlock_t *);
int   pthread_rwlock_wrlock(pthread_rwlock_t *);


int   pthread_rwlockattr_destroy(pthread_rwlockattr_t *);
int   pthread_rwlockattr_getpshared(
         const pthread_rwlockattr_t *restrict, int *restrict);
int   pthread_rwlockattr_init(pthread_rwlockattr_t *);
int   pthread_rwlockattr_setpshared(pthread_rwlockattr_t *, int);
#endif

#if 1

int   pthread_spin_destroy(pthread_spinlock_t *);
int   pthread_spin_init(pthread_spinlock_t *, int);
int   pthread_spin_lock(pthread_spinlock_t *);
int   pthread_spin_trylock(pthread_spinlock_t *);
int   pthread_spin_unlock(pthread_spinlock_t *);
#endif

#if 1
int   pthread_key_create(pthread_key_t *, void (*)(void*));
int   pthread_key_delete(pthread_key_t);
int   pthread_setspecific(pthread_key_t, const void *);
void *pthread_getspecific(pthread_key_t);


#endif

#if 1
int   pthread_getschedparam(pthread_t, int *restrict,
         struct sched_param *restrict);
int   pthread_setschedparam(pthread_t, int,
         const struct sched_param *);
int   pthread_setschedprio(pthread_t, int);


int   pthread_atfork(void (*)(void), void (*)(void), void(*)(void));
int   pthread_create(pthread_t *restrict, const pthread_attr_t *restrict,
         void *(*)(void*), void *restrict);
int   pthread_once(pthread_once_t *, void (*)(void));

int   pthread_join(pthread_t, void **);
int   pthread_detach(pthread_t);

int   pthread_getcpuclockid(pthread_t, clockid_t *);

pthread_t pthread_self(void);

int   pthread_equal(pthread_t, pthread_t);


#endif

#if 1

int   pthread_setcancelstate(int, int *);
int   pthread_setcanceltype(int, int *);
void  pthread_testcancel(void);
int   pthread_cancel(pthread_t);
void pthread_cleanup_push(void (*routine)(void *), void *arg);
void pthread_cleanup_pop(int execute);

void  pthread_exit(void *);

#endif

#if 1

/* Both LinuxThreads and NPTL are 1:1 threading implementations, so
       setting the concurrency level has no meaning.  In other words, on
       Linux these functions merely exist for compatibility with other
       systems, and they have no effect on the execution of a program. */
int   pthread_getconcurrency(void);
int   pthread_setconcurrency(int);

#endif



