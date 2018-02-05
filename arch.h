#ifndef ARCH_H
#define ARCH_H

#include <stdio.h>

#include "types.h"

// 32/64 bits
//#define BITS_PER_LONG   32
#define BITS_PER_LONG   64

// cache line size in bytes
#define CACHE_LINE      64

// big/small endian
//#define LITTLE_ENDIAN

#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
// barrier() prevent compiler re-ordering memory across barrier
#define barrier()       asm volatile("" : : : "memory");
// Use offsetof() to get the offset of struct member
//#define offsetof(type, member) (size_t)&(((type *)0)->member)
// use likely() and unlikely() to provide the compiler with branch prediction information
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

// prefetch() indicates to the processor that a cache line will be needed in the near future.
#define prefetch(p)     __builtin_prefetch(p)
// aligned_member
#define __align_cache   __attribute__ ((aligned (CACHE_LINE)))

// aligned_malloc() allocates memory on a specified alignment boundary
static inline void *aligned_malloc(size_t size, size_t alignment) {
    void *memptr;
    if(posix_memalign(&memptr, alignment, size) != 0)
        return 0;
    return memptr;
}

// aligned_malloc() frees a block of memory that was allocated with _aligned_malloc
#define aligned_free(ptr)   free(ptr)

// mb() barrier ensures that memory read and write operations that appear before the memory
// barrier instruction are committed to memory (not necessary immediately) before any memory
// read and write operations that appear after the memory barrier instruction. A read memory
// barrier orders only the memory read operations and a write memory barrier orders only the
// memory write operations.
#define mb()        asm volatile("mfence":::"memory")
#define rmb()       asm volatile("lfence":::"memory")
#define wmb()       asm volatile("sfence"::: "memory")
#define smp_mb()    mb()
#define smp_rmb()   rmb()
#define smp_wmb()   wmb()

#define mc()        mb()
#define rmc()       rmb()
#define wmc()       wmb()
#define smp_mc()    mc()
#define smp_rmc()   rmc()
#define smp_wmc()   wmc()

// rwlock_t operations
typedef pthread_rwlock_t        rwlock_t;
#define DEFINE_RWLOCK(x)        rwlock_t x = PTHREAD_RWLOCK_INITIALIZER
#define rwlock_init(lock)       pthread_rwlock_init(lock, NULL)
#define read_trylock(lock)      pthread_rwlock_tryrdlock(lock)
#define read_lock(lock)         pthread_rwlock_rdlock(lock)
#define read_unlock(lock)       pthread_rwlock_unlock(lock)
#define write_trylock(lock)     pthread_rwlock_trywrlock(lock)
#define write_lock(lock)        pthread_rwlock_wrlock(lock)
#define write_unlock(lock)      pthread_rwlock_unlock(lock)


// spin_lock_t operations
typedef pthread_spinlock_t  spinlock_t;
// macro to delcare and initialize spinlock
#define spin_lock_init(lock)    pthread_spin_init(lock, PTHREAD_PROCESS_PRIVATE)
#define spin_trylock(lock)      pthread_spin_trylock(lock)
#define spin_lock(lock)         pthread_spin_lock(lock)
#define spin_unlock(lock)       pthread_spin_unlock(lock)

// atomic operations
static inline void *atomic_xchg_ptr(void **v, void *new) {
    return __sync_lock_test_and_set(v, new);
}

static __inline__ int atomic_bit_test_and_set(u32 *ptr, u32 offset)
{
    u32 retval, val=1<<offset;

    retval = __sync_fetch_and_or(ptr, val) & val;

    return retval;
}

// Instruct the compiler to perform only a single access to a variable (prohibits merging and refetching).
#define ACCESS_ONCE(x)          (*(volatile typeof(x) *)&x)
// Get the container address according to member address &ptr
#define container_of(ptr, type, member) ((type *)( (char *)(ptr) - offsetof(type, member) ))

// network <-> host translation
#define ntoh32(v)   ntohl(v)
#define hton32(v)   htonl(v)
#define ntoh16(v)   ntohs(v)
#define hton16(v)   htons(v)

#ifdef LITTLE_ENDIAN
    #define hton64(host)    __builtin_bswap64(host)
    #define ntoh64(host)    __builtin_bswap64(host)
#else   // BIG_ENDIAN
    #define ntoh64(net)     (net)
    #define hton64(host)    (host)
#endif

// error handling
#define SUCCESS     1
#define FAIL        0
#ifndef TRUE
#define TRUE        1
#endif
#ifndef FALSE
#define FALSE       0
#endif

#define MYASSERT(cond)  if (!(cond)) {  printf("ERROR:%s(): %d\n", __func__, __LINE__); return FAIL; }

// for dump
static inline void printSpace(int num)
{
    while (num>0) {
        printf(" ");
        num--;
    }
}

#endif  // ARCH_H
