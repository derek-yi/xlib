#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>        /* Definition of uint64_t */

#define handle_error(msg) \
        do { perror(msg); exit(EXIT_FAILURE); } while (0)

static void
print_elapsed_time(void)
{
    static struct timespec start;
    struct timespec curr;
    static int first_call = 1;
    int secs, nsecs;

   if (first_call) {
        first_call = 0;
        if (clock_gettime(CLOCK_MONOTONIC, &start) == -1)
            handle_error("clock_gettime");
    }

   if (clock_gettime(CLOCK_MONOTONIC, &curr) == -1)
        handle_error("clock_gettime");

   secs = curr.tv_sec - start.tv_sec;
    nsecs = curr.tv_nsec - start.tv_nsec;
    if (nsecs < 0) {
        secs--;
        nsecs += 1000000000;
    }
    printf("%d.%03d: ", secs, (nsecs + 500000) / 1000000);
}

int
main(int argc, char *argv[])
{
    struct itimerspec new_value;
    struct timespec now;
    int interval, delay, fd;
    uint64_t exp, tot_exp;
    ssize_t s;

   if (argc != 3) {
        fprintf(stderr, "%s <interval-ms> <delay-ms>\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

   /* Create a CLOCK_REALTIME absolute timer with initial
       expiration and interval as specified in command line */
   if (clock_gettime(CLOCK_REALTIME, &now) == -1)
        handle_error("clock_gettime");

    interval = atoi(argv[1]);
    delay = atoi(argv[2]);
    new_value.it_value.tv_sec = now.tv_sec + interval/1000;
    new_value.it_value.tv_nsec = now.tv_nsec + (interval%1000)*1000*1000;
    new_value.it_interval.tv_sec = now.tv_sec + interval/1000;
    new_value.it_interval.tv_sec = now.tv_nsec + (interval%1000)*1000*1000;

   fd = timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC);
   if (fd == -1)
        handle_error("timerfd_create");

   if (timerfd_settime(fd, TFD_TIMER_ABSTIME, &new_value, NULL) == -1)
        handle_error("timerfd_settime");

   print_elapsed_time();
   printf("timer started\n");

   while (1) {
        s = read(fd, &exp, sizeof(uint64_t));
        if (s != sizeof(uint64_t))
            handle_error("read");

        tot_exp += exp;
        print_elapsed_time();
        printf("exp %llu, tot_exp %llu\n",
                (unsigned long long) exp,
                (unsigned long long) tot_exp);
        if (delay) usleep(delay * 1000);
    }

   exit(EXIT_SUCCESS);
}
