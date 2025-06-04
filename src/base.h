/*
 *  Copyright (C) 2022-2025 Nicolai Brand (lytix.dev)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef BASE_H
#define BASE_H

/*
 * Copy of README:
 *
 * Amalgamation of useful C functions, datastructures, allocators, and more.
 *
 * Contains:
 * -> sac: Simple Arena Allocator
 * -> nicc: Datastructures (HashMap, ArrayList, LinkedList)
 * -> str: Pascal-like strings with a bunch of helper functions. Yoinked from the metagen project.
 * -> log: Logging. Also yoinked from the metagen project.
 * -> nag: Nicolai's Amazing Graph library for directed graphs.
 * -> threadpool (tp)
 *
 *
 * Every C project I work on invariably ends up using two or more of the above projects. To make it
 * easier to maintain and use, I merged them into a single header file. My own standard library of
 * sorts.
 */

#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h> // for size_t and ssize_t
#include <string.h>
#include <sys/mman.h> // sac.h
#include <sys/types.h>
#include <unistd.h>


/*
 *  Originally from type.h
 */
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;
typedef double f64;

#define U8_MIN 0u
#define U8_MAX 0xffu
#define S8_MIN (-0x7f - 1)
#define S8_MAX 0x7f

#define U16_MIN 0u
#define U16_MAX 0xffffu
#define S16_MIN (-0x8000)
#define S16_MAX 0x7fff

#define U32_MIN 0u
#define U32_MAX 0xffffffffu
#define S32_MIN (-0x7fffffff - 1)
#define S32_MAX 0x7fffffff

#define U64_MIN 0ull
#define U64_MAX 0xffffffffffffffffull
#define S64_MIN (-0x7fffffffffffffffll - 1)
#define S64_MAX 0x7fffffffffffffffll


#define ASSERT_NOT_REACHED assert(0 && "panic: unreachable code reached")

#define VA_NUMBER_OF_ARGS(...) (sizeof((int[]){ __VA_ARGS__ }) / sizeof(int))

#define ARRAY_LENGTH(arr) (size_t)(sizeof(arr) / sizeof((arr)[0]))

#define IS_BETWEEN(x, lower, upper) (((lower) <= (x)) && ((x) <= (upper)))

// TODO: we assume NULL is 0x0 in many places. This holds true for every compiler I've ever seen,
//       but its not guaranteed. So we should give an error if we detect it is in fact not 0.
//       Same is true for page size. sac_single assumes a page size of 4096. Usually true, not
//       guaranteed.

/*
 * Originally from sac_single.h
 */

#ifndef __unix__
#define SAC_BAD_AARCH
#endif

/* Try to define SAC_MAP_ANON */
#ifdef MAP_ANON
#define SAC_MAP_ANON MAP_ANON
#endif /* MAP_ANON */

#ifndef SAC_MAP_ANON
#ifdef MAP_ANONYMOUS
#define SAC_MAP_ANON MAP_ANONYMOUS
#endif /* MAP_ANONYMOUS */
#endif /* SAC_MAP_ANON */

#ifndef SAC_MAP_ANON
#define SAC_BAD_AARCH
#endif /* SAC_MAP_ANON */

#ifndef SAC_DEFAULT_ALIGNMENT
#define SAC_DEFAULT_ALIGNMENT (sizeof(void *))
#endif

typedef struct m_arena Arena;
typedef struct m_arena_tmp ArenaTmp;

/*
 * generic memory arena that dynamically grows its committed size.
 * more complex memory arenas can be built using this as a base.
 */
struct m_arena {
    uint8_t *memory; // the backing memory
    size_t offset; // first unused position in the backing memory
    bool is_dynamic;
    union {
        size_t max_pages;
        size_t backing_length;
    };
    size_t page_size;
    size_t pages_commited; // how much of the backing memory is acutally "backing"
};

struct m_arena_tmp {
    struct m_arena *arena;
    size_t offset;
};


/* functions */
void m_arena_init(struct m_arena *arena, void *backing_memory, size_t backing_length);
void m_arena_init_dynamic(struct m_arena *arena, size_t starting_pages, size_t max_pages);
void m_arena_release(struct m_arena *arena);

void *m_arena_alloc_internal(struct m_arena *arena, size_t size, size_t align, bool zero);
#define m_arena_alloc(arena, size) m_arena_alloc_internal(arena, size, SAC_DEFAULT_ALIGNMENT, false)
#define m_arena_alloc_zero(arena, size) \
    m_arena_alloc_internal(arena, size, SAC_DEFAULT_ALIGNMENT, true)

void m_arena_clear(struct m_arena *arena);
void *m_arena_get(struct m_arena *arena, size_t byte_idx);

#define m_arena_alloc_array(arena, type, count) \
    (type *)m_arena_alloc((arena), sizeof(type) * (count))
#define m_arena_alloc_array_zero(arena, type, count) \
    (type *)m_arena_alloc_zero((arena), sizeof(type) * (count))
#define m_arena_alloc_struct(arena, type) (type *)m_arena_alloc((arena), sizeof(type))
#define m_arena_alloc_struct_zero(arena, type) (type *)m_arena_alloc_zero((arena), sizeof(type))
#define m_arena_gett(arena, idx, type) (type *)m_arena_get((arena), sizeof(type) * (idx))


struct m_arena_tmp m_arena_tmp_init(struct m_arena *arena);
void m_arena_tmp_release(struct m_arena_tmp tmp);
/* cursed */
#define ARENA_TMP(___arena)                                          \
    for (size_t ___i = 0, ___offset = (___arena)->offset; ___i == 0; \
         ___i += 1, (___arena)->offset = ___offset)

/*
 * Originally from nicc.h
 */
#ifndef NICC_NOT_FOUND
#define NICC_NOT_FOUND SIZE_MAX
#endif

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, new_size) \
    (type *)nicc_internal_realloc((pointer), sizeof(type) * (new_size))

/* internal function definitions */
void *nicc_internal_realloc(void *ptr, size_t new_size);

bool nicc_data_eq(void *a, void *b, u32 T_size);

typedef s32 compare_fn_t(const void *, const void *);

typedef bool equality_fn_t(const void *, const void *);


#define BYTE_SWAP(a, b, size)                    \
    do {                                         \
        size_t __size = (size);                  \
        register unsigned char *__a = (a);       \
        register unsigned char *__b = (b);       \
        do {                                     \
            register unsigned char __tmp = *__a; \
            *__a++ = *__b;                       \
            *__b++ = __tmp;                      \
        } while (--__size > 0);                  \
    } while (0)

typedef struct arraylist_t ArrayList;

struct arraylist_t {
    void *data;
    u32 T_size;
    size_t size;
    size_t cap;
};

void arraylist_init(struct arraylist_t *arr, u32 T_size);
void arraylist_free(struct arraylist_t *arr);

bool arraylist_set(struct arraylist_t *arr, void *val, size_t idx);
bool arraylist_append(struct arraylist_t *arr, void *val);

void *arraylist_get(struct arraylist_t *arr, size_t idx);
void arraylist_get_copy(struct arraylist_t *arr, size_t idx, void *return_ptr);
bool arraylist_pop(struct arraylist_t *arr);
bool arraylist_pop_and_copy(struct arraylist_t *arr, void *return_ptr);

size_t arraylist_index_of(struct arraylist_t *arr, void *val, equality_fn_t *eq);

bool arraylist_rm(struct arraylist_t *arr, size_t idx);
bool arraylist_rmv(struct arraylist_t *arr, void *val, equality_fn_t *eq);

bool arraylist_sort(struct arraylist_t *arr, compare_fn_t *cmp);

/* return codes for insert() function */
#define _HM_FULL 1
#define _HM_OVERRIDE 2
#define _HM_SUCCESS 3

#define HM_STARTING_BUCKETS_LOG2 3 // the amount of starting buckets
#define HM_BUCKET_SIZE 6
#define HM_OVERFLOW_SIZE 4
#define N_BUCKETS(log2) (1 << (log2))

/*
 * Quick note on the hashmap:
 * The hashmap impl. is a pretty standard dynamically growing hashmap. Every
 * bucket stores 6 entries by default. The aim of the hashmap implementation is
 * to be cache efficient and simple to use for any datatype and at any scale. It
 * takes major inspiration from the hashmap implemented in Go. NICC also
 * includes a fixed size linked-list hashtable called `ht`. The cache properties
 * of the fixed sized hashtable (ht) are likely to be noticably worse due to the
 * nature of linked list. It is also unlikely to scale particularly well.
 *
 * The hashmap implementation was originally written to be a part of a
 * collaborative project with a friend of mine. It can also be found here
 * amongst some examples and correctness tests:
 * https://github.com/DHPS-Solutions/dhps-lib
 */

struct hm_entry_t {
    void *key; // if NULL then entry is considered unused
    void *value;
    u32 key_size;
    u32 value_size;
    u8 hash_extra; // used for faster comparison
    u8 alloc_flag; // true if value is alloced
                   // if sizeof bool and _Bool > 3 then this struct would be of
                   // size 36 instead 32 due to struct padding. Therefore we use
                   // one byte to be safe we don't waste any memory space.
};

struct hm_bucket_t {
    struct hm_entry_t entries[HM_BUCKET_SIZE];
};

typedef struct hashmap_t HashMap;

struct hashmap_t {
    struct hm_bucket_t *buckets;
    u8 size_log2;
    u32 len; // total items stored in the hashmap
    // #ifdef HASHMAP_THREAD_SAFE
    //     pthread_mutex_t lock;
    // #endif
};

void hashmap_init(struct hashmap_t *map);

void hashmap_free(struct hashmap_t *map);

void hashmap_put(struct hashmap_t *map, void *key, u32 key_size, void *value, u32 val_size,
                 bool alloc_flag);
#define hashmap_sput(map, key, value, val_size, alloc_flag) \
    hashmap_put(map, key, (strlen(key) + 1) * sizeof(char), value, val_size, alloc_flag)
#define hashmap_ssput(map, key, value, alloc_flag)                 \
    hashmap_put(map, key, (strlen(key) + 1) * sizeof(char), value, \
                (strlen(value) + 1) * sizeof(char), alloc_flag)

void *hashmap_get(struct hashmap_t *map, void *key, u32 key_size);
#define hashmap_sget(map, key) hashmap_get(map, key, (strlen(key) + 1) * sizeof(char))

bool hashmap_rm(struct hashmap_t *map, void *key, u32 key_size);
#define hashmap_srm(map, key) hashmap_rm(map, key, (strlen(key) + 1) * sizeof(char))

/*
 * The length of return_ptr must be at least sizeof(void *) * map->len bytes.
 * Anything less becomes UB.
 */
void hashmap_get_values(struct hashmap_t *map, void **return_ptr);

/*
 * The length of return_ptr must be at least sizeof(void *) * map->len bytes.
 * Anything less becomes UB.
 */
void hashmap_get_keys(struct hashmap_t *map, void **return_ptr);

/* heap queue inspired by: https://docs.python.org/3/library/heapq.html */
struct heapq_t {
    void **items;
    int size;
    int capacity;
    compare_fn_t *cmp;
};

void heapq_init(struct heapq_t *hq, compare_fn_t *cmp);
void heapq_free(struct heapq_t *hq);
void heapq_push(struct heapq_t *hq, void *item);
void *heapq_get(struct heapq_t *hq, int idx);
/*
 * returns and removes the item at the top of the heap queue.
 * note: remember to free() the popped item after use if it was malloced before
 * pushing into the heapq.
 */
void *heapq_pop(struct heapq_t *hq);
void heap_sort(const void *base, size_t nmemb, size_t size, compare_fn_t *cmp);

typedef struct linkedlist_t LinkedList;
typedef struct linkedlist_item_t LinkedListItem;

struct linkedlist_item_t {
    void *data; // pointer to the data. we don't manage the memory of this.
    struct linkedlist_item_t *prev; // can be NULL
    struct linkedlist_item_t *next; // can be NULL
};

/*
 * Doubly linked list implementation.
 * Assumes all data that is stored has the same length.
 */
struct linkedlist_t {
    struct linkedlist_item_t *head;
    struct linkedlist_item_t *tail;
    size_t size; // for convenience
    u32 T_size; // sizeof the value that data in linkedlist_item_t points to
};

/* Same as `struct linkedlist_t ll = { .head = NULL, .tail = NULL, . size = 0, .T_size = T_size }`
 */
void linkedlist_init(struct linkedlist_t *ll, u32 T_size);
/* Frees all the connected items in the linkedlist */
void linkedlist_free(struct linkedlist_t *ll);
void linkedlist_append(struct linkedlist_t *ll, void *data);
/* Removes the provided item and updates underlying linkedlist container if needed */
void linkedlist_remove_item(struct linkedlist_t *ll, struct linkedlist_item_t *to_remove);
/* Removes the first occurence of data. */
bool linkedlist_remove(struct linkedlist_t *ll, void *data);
bool linkedlist_remove_idx(struct linkedlist_t *ll, size_t idx);
void linkedlist_print(struct linkedlist_t *ll);
#define NICC_LL_FOR_EACH(ll, item) for (item = (ll)->head; item != NULL; item = item->next)

/*
 * Originally from str.h
 */
/*
 * Should always be null terminated.
 * Size does not include null terminator.
 */
typedef struct {
    size_t len;
    u8 *str;
} Str8;

// TODO: VLA considered harmful, but maybe this is more ergonomic?
// typedef struct {
//     size_t len;
//     u8 str[]; // length + 1
// } Str8;

typedef struct {
    Arena *arena;
    Str8 str;
    u32 cap;
} Str8Builder;

typedef struct {
    Str8 *strs; // On the heap.
    u32 len;
    u32 cap;
} Str8List;

/*
 * A view into memory.
 * Not guaranteed to be null terminated.
 */
typedef Str8 Str8View;

/* Used with %.*s */
#define STR8VIEW_PRINT(view) (int)(view).len, (const char *)(view).str
/* Str8View from a literal */
#define STR8VIEW_LIT(literal)                \
    (Str8View)                               \
    {                                        \
        sizeof(literal) - 1, (u8 *)(literal) \
    }
#define STR8VIEW_EQUAL(a, b) \
    ((a).len == (b).len && ((a).len == 0 || memcmp((a).str, (b).str, (a).len) == 0))
#define STR8_EQUAL(a, b) (STR8VIEW_EQUAL((a), (b)))

#define STR8_LIT(literal)                    \
    (Str8)                                   \
    {                                        \
        sizeof(literal) - 1, (u8 *)(literal) \
    }


u32 str_view_to_u32(Str8View view, bool *success);

Str8Builder make_str_builder(Arena *arena);
void str_builder_append_u8(Str8Builder *sb, u8 c);
void str_builder_append_cstr(Str8Builder *sb, char *cstr, u32 len);
void str_builder_append_str8(Str8Builder *sb, Str8 str);
void str_builder_sprintf(Str8Builder *sb, char *fmt, int count, ...);
Str8 str_builder_end(Str8Builder *sb, bool add_null_terminator);

void str_list_init(Str8List *list);
void str_list_free(Str8List *list);
u32 str_list_push(Str8List *list, Str8 str);
u32 str_list_push_cstr(Arena *arena, Str8List *list, char *cstr);
void str_list_print(Str8List *list);
/* The individual strings in the list will be Str8Views into the original Str8 input ! */
Str8List str_list_from_split(Str8 input, char delim);

/*
 * Originally from log.h
 */
typedef enum {
    LOG_ERR = 0,
    LOG_WARN = 1,
    LOG_ALL = 2 /* Debug mode */
} LogLevel;


typedef struct logger_t {
    LogLevel log_level;
    // TODO: Also store the logged messages??
} Logger;


/* Global logging */
void log_init_global(LogLevel log_level);

#define LOG_WARN(fmt, ...) log_bad_event(LOG_WARN, "WARNING", (fmt), ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) log_bad_event(LOG_ERR, "ERROR", (fmt), ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) log_bad_event(LOG_ERR, "FATAL", (fmt), ##__VA_ARGS__)
#define LOG_WARN_NOARG(msg) log_bad_event(LOG_WARN, "WARNING", "%s", msg)
#define LOG_ERROR_NOARG(msg) log_bad_event(LOG_ERR, "ERROR", "%s", msg)
#define LOG_FATAL_NOARG(msg) log_bad_event(LOG_ERR, "FATAL", "%s", msg)
void log_bad_event(LogLevel level, char *prefix, char *fmt, ...);

#define LOG_DEBUG_NOARG(msg) log_debug(__FILE__, __LINE__, false, msg)
#define LOG_FIXME_NOARG(msg) log_debug(__FILE__, __LINE__, true, msg)
#define LOG_DEBUG(msg, ...) log_debug(__FILE__, __LINE__, false, (msg), ##__VA_ARGS__)
#define LOG_FIXME(msg, ...) log_debug(__FILE__, __LINE__, true, (msg), ##__VA_ARGS__)
void log_debug(char *file, size_t line, bool is_a_fixme, char *fmt, ...);
void log_debug_str8(Str8 msg, char *file, size_t line, bool is_a_fixme);

/*
 * Originally from nag.h
 */
/*
 * The index type.
 * If u16 does not suffice, just change this typedef.
 */
typedef u16 NAG_Idx;

#define NAG_STACK_GROW_SIZE (NAG_Idx)256 // at least 8
#define NAG_QUEUE_GROW_SIZE \
    (NAG_Idx)32 // at least 8
                //
#define NAG_MIN(a, b) ((a) < (b) ? (a) : (b))

#define NAG_UNDISCOVERED U16_MAX

typedef struct nag_graph_node_t NAG_GraphNode;
struct nag_graph_node_t {
    NAG_Idx id;
    NAG_GraphNode *next;
};

typedef struct {
    NAG_Idx n_nodes;
    NAG_GraphNode **neighbor_list;
    Arena *scratch_arena;
    Arena *persist_arena;
} NAG_Graph;

typedef struct {
    NAG_Idx n_nodes;
    NAG_Idx *nodes; // of n_nodes len
} NAG_Order;

typedef struct {
    u32 n; // how many orders
    NAG_Order *orders; // NOTE: Heap allocated!
} NAG_OrderList;


NAG_Graph nag_make_graph(Arena *persist, Arena *scratch, NAG_Idx n_nodes);
/* Expects node indices between 0 and graph->n_nodes - 1 */
void nag_add_edge(NAG_Graph *graph, NAG_Idx from, NAG_Idx to);
void nag_print(NAG_Graph *graph);

NAG_OrderList nag_dfs(NAG_Graph *graph);
NAG_Order nag_dfs_from(NAG_Graph *graph, NAG_Idx start_node);

NAG_OrderList nag_bfs(NAG_Graph *graph);
NAG_Order nag_bfs_from(NAG_Graph *graph, NAG_Idx start_node);

/* Assumes graph contains no cycles */
NAG_Order nag_rev_toposort(NAG_Graph *graph);

NAG_OrderList nag_scc(NAG_Graph *graph);

/*
 * Originally from threadpool.h
 */
typedef void *(*TP_Function)(void *);

#define MAP_LOAD_FACTOR_PERCENT 70

typedef u32 TP_TaskHandle;
// Task was submitted successfully to the queue, but the handle is not used
#define TASK_HANDLE_UNUSED 0
// Task was not successfully submitted as the queue was full
#define TASK_HANDLE_FULL U32_MAX

// NOTE: Could it be useful to allow a callback that runs when the task is finished?
typedef struct {
    TP_TaskHandle handle;
    TP_Function func;
    void *args;
} TP_Task;

typedef enum { TASK_NOT_FOUND, TASK_IN_QUEUE, TASK_RUNNING, TASK_FINISHED } TP_TaskStatus;

typedef struct {
    TP_TaskStatus status;
    TP_Task task;
    void *result; // @NULLABLE
} TP_TaskInfo;

typedef struct {
    TP_TaskHandle *keys; // If entry is unoccupied or deleted, value is TASK_HANDLE_UNUSED
    TP_TaskInfo *values;
    u32 slots_allocated;
    u32 slots_filled; // Number of active entries (i.e no unoccupied or deleted entries)
} TP_TaskInfoMap;


/* FIFO queue implemented as a ring buffer */
typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t cv;
    TP_Task *tasks;
    u32 size;
    u32 head; // Index to the next task to be handled
    u32 tail; // Index after the most recently queued task
} TP_TaskQueue;

typedef struct threadpool_t {
    u32 n_threads;
    pthread_t *threads;
    TP_TaskQueue task_queue;

    TP_TaskHandle task_handle_counter;
    TP_TaskInfoMap task_info_map;

    bool join; // Wait for all running and queued tasks to finish
    bool stop_new; // Stop new tasks from being started, but let threads finish what they're doing
} TP_ThreadPool;


/* Functions */
void tp_init(TP_ThreadPool *tp, u32 n_threads, u32 queue_size);
void tp_destroy(TP_ThreadPool *tp);

void tp_start(TP_ThreadPool *tp);
void tp_join(TP_ThreadPool *tp);
void tp_stop_new(TP_ThreadPool *tp);

/*
 * If wait_if_full then the the function wait until the queue has space for the task
 *
 * If set_up_handle then the function returns a valid handle that later can be used to get the
 *  info of the task using tp_get_info(). NOTE: make sure that the TP_Function store its return
 *  value somewhere other than the stack so it does not turn into garbage.
 */
TP_TaskHandle tp_queue_task(TP_ThreadPool *tp, TP_Function f, void *args, bool wait_if_full,
                            bool set_up_handle);

/*
 * If remove_if_finished then the threadpool will internally forget about the handle and repurpose
 *  the memory.
 */
TP_TaskInfo tp_get_info(TP_ThreadPool *tp, TP_TaskHandle handle, bool remove_if_finished);

#endif /* BASE_H */


/*
 *
 * Implementation
 *
 */
#if defined(BASE_IMPLEMENTATION) && !defined(BASE_IMPLEMENTATION_INCLUDED)
#define BASE_IMPLEMENTATION_INCLUDED

/*
 * Originally from sac_single.h
 */
/* internal functions */
static bool m_arena_commit(struct m_arena *arena, size_t pages_to_commit)
{
    assert(arena->is_dynamic);

    /* no more space to commit */
    if (arena->pages_commited + pages_to_commit > arena->max_pages)
        return false;

    int rc = mprotect(arena->memory + (arena->pages_commited * arena->page_size),
                      pages_to_commit * arena->page_size, PROT_READ | PROT_WRITE);
    if (rc == -1)
        return false;

    arena->pages_commited += pages_to_commit;
    return true;
}

static bool m_arena_ensure_commited(struct m_arena *arena)
{
    if (!arena->is_dynamic)
        return arena->offset <= arena->backing_length;

    /* know arena is dynamic */
    size_t memory_committed = arena->pages_commited * arena->page_size;
    if (arena->offset <= memory_committed)
        return true;

    // TODO: this math can probably be optimized
    size_t delta = arena->offset - memory_committed;
    size_t pages_to_commit = (delta / arena->page_size) + 1;
    if (arena->pages_commited + pages_to_commit > arena->max_pages)
        return false;

    m_arena_commit(arena, pages_to_commit);
    return true;
}

/*
 * stolen from:
 * https://www.gingerbill.org/article/2019/02/08/memory-allocation-strategies-002/
 */
static bool is_power_of_two(uintptr_t x)
{
    return (x & (x - 1)) == 0;
}

/*
 * slightly modified, but mostly stolen from:
 * https://www.gingerbill.org/article/2019/02/08/memory-allocation-strategies-002/
 */
static uintptr_t align_forward(uintptr_t ptr, size_t align)
{
    assert(is_power_of_two(align));

    uintptr_t p = ptr;
    uintptr_t a = (uintptr_t)align;
    uintptr_t modulo = p & (a - 1);

    if (modulo != 0)
        p += a - modulo;

    return p;
}

/* SAC functions */
void m_arena_init(struct m_arena *arena, void *backing_memory, size_t backing_length)
{
    assert(backing_memory != NULL && backing_length > 0);

    arena->is_dynamic = false;
    arena->memory = backing_memory;
    arena->backing_length = backing_length;
    arena->offset = 0;
}

void m_arena_init_dynamic(struct m_arena *arena, size_t starting_pages, size_t max_pages)
{
    assert(starting_pages <= max_pages);

    arena->is_dynamic = true;
    arena->max_pages = max_pages;
    arena->page_size = sysconf(_SC_PAGE_SIZE);
    arena->offset = 0;
    arena->pages_commited = 0;

    arena->memory = mmap(NULL, arena->max_pages * arena->page_size, PROT_NONE,
                         MAP_PRIVATE | SAC_MAP_ANON, -1, 0);
    if (arena->memory == MAP_FAILED) {
        fprintf(stderr, "sac: map failed in file %s on line %d\n", __FILE__, __LINE__);
        exit(1);
    }

    if (starting_pages != 0)
        m_arena_commit(arena, starting_pages);
}

void m_arena_release(struct m_arena *arena)
{
    assert(arena != NULL);

    /* the implementation does not manage the backing memory */
    if (!arena->is_dynamic)

        munmap(arena->memory, arena->max_pages);
}

/*
 * heavily modified, but inspired by:
 * https://www.gingerbill.org/article/2019/02/08/memory-allocation-strategies-002/
 */
void *m_arena_alloc_internal(struct m_arena *arena, size_t size, size_t alignment, bool zero)
{
    /* curr_ptr will be the first non-used memory address */
    uintptr_t curr_ptr = (uintptr_t)arena->memory + (uintptr_t)arena->offset;
    /* offset wil be the first aligned to one word non-used memory adress */
    uintptr_t offset = align_forward(curr_ptr, alignment);
    /* change to relative offset from the first memory adress */
    offset -= (uintptr_t)arena->memory;
    arena->offset = offset + size;

    bool success = m_arena_ensure_commited(arena);
    if (!success)
        return NULL;

    void *ptr = arena->memory + offset;
    if (zero)
        memset(ptr, 0, size);

    return ptr;
}

void m_arena_clear(struct m_arena *arena)
{
    arena->offset = 0;
}

void *m_arena_get(struct m_arena *arena, size_t byte_idx)
{
    if (byte_idx > arena->offset)
        return NULL;

    return arena->memory + byte_idx;
}

struct m_arena_tmp m_arena_tmp_init(struct m_arena *arena)
{
    return (struct m_arena_tmp){ .arena = arena, .offset = arena->offset };
}

void m_arena_tmp_release(struct m_arena_tmp tmp)
{
    tmp.arena->offset = tmp.offset;
}

/*
 * Originally from nicc.c
 */
void *nicc_internal_realloc(void *ptr, size_t new_size)
{
    void *res = realloc(ptr, new_size);
    // TODO: better error handling
    if (res == NULL)
        exit(1);

    return res;
}

bool nicc_data_eq(void *a, void *b, u32 T_size)
{
    /* bytewise comparison */
    u8 A;
    u8 B;
    for (size_t i = 0; i < (size_t)T_size; i++) {
        A = ((u8 *)a)[i];
        B = ((u8 *)b)[i];
        if (A != B)
            return false;
    }
    return true;
}

static u32 hash_func_m(void *data, u32 size)
{
    /* gigahafting kok (legger dermed ikke så mye lit til det) */
    u32 A = 1327217885;
    u32 k = 0;
    for (u32 i = 0; i < size; i++)
        k += (k << 5) + ((u8 *)data)[i];

    return k * A;
}

static inline u8 hm_hash_extra(u32 hash)
{
    return (u8)(hash & ((1 << 8) - 1));
}

static inline void entry_free(struct hm_entry_t *entry)
{
    free(entry->key);
    if (entry->alloc_flag)
        free(entry->value);
}

static inline void insert_entry(struct hm_entry_t *found, struct hm_entry_t *new)
{
    // TODO: should we just set the value of found to be the value of new?

    if (found->key != NULL)
        free(found->key);
    found->key = malloc(new->key_size);
    memcpy(found->key, new->key, new->key_size);

    /*
     * if already alloced space is sufficient, use that
     * if space is not sufficient, realloc
     */
    if (!new->alloc_flag) {
        if (found->alloc_flag)
            free(found->value);
        found->value = new->value;
    } else {
        if (!found->alloc_flag)
            found->value = malloc(new->value_size);
        else if (new->value_size > found->value_size)
            found->value = realloc(found->value, new->value_size);
        memcpy(found->value, new->value, new->value_size);
    }

    found->key_size = new->key_size;
    found->value_size = new->value_size;
    found->hash_extra = new->hash_extra;
    found->alloc_flag = new->alloc_flag;
}

static int insert(struct hm_bucket_t *bucket, struct hm_entry_t *new)
{
    /*
     * Our hashmap implementation does not allow duplcate keys.
     * Therefore, we cannot simply find the first empty entry and set the new
     * entry here, we have to make sure the key we are inserting does not already
     * exist somewhere else in the bucket. If we find a matching key, we simply
     * override. If we do not find a matching entry, we insert the new entry in
     * the first found empty entry.
     */
    struct hm_entry_t *found = NULL;
    bool override = false;
    for (u8 i = 0; i < HM_BUCKET_SIZE; i++) {
        // TODO må jeg ta peker?
        struct hm_entry_t *entry = &bucket->entries[i];
        if (entry->key != NULL &&
            new->hash_extra == entry->hash_extra &&new->key_size == entry->key_size) {
            if (memcmp(new->key, entry->key, new->key_size) == 0) {
                found = entry;
                override = true;
                break;
            }
        }

        if (found == NULL && entry->key == NULL)
            found = entry;
    }

    if (found == NULL)
        return _HM_FULL;

    insert_entry(found, new);
    return override ? _HM_OVERRIDE : _HM_SUCCESS;
}

static struct hm_entry_t *get_from_bucket(struct hm_bucket_t *bucket, void *key, u32 key_size,
                                          u8 hash_extra)
{
    struct hm_entry_t entry;
    for (u8 i = 0; i < HM_BUCKET_SIZE; i++) {
        entry = bucket->entries[i];
        if (entry.key == NULL)
            continue;
        if (key_size == entry.key_size && hash_extra == entry.hash_extra) {
            // TODO: instead of memcmp, use a function that exits if a byte is not
            // equal
            if (memcmp(key, entry.key, key_size) == 0)
                return &bucket->entries[i];
        }
    }
    return NULL;
}

static struct hm_entry_t *get_entry(struct hashmap_t *map, void *key, u32 key_size)
{
    if (map->len == 0)
        return NULL;

    u32 hash = hash_func_m(key, key_size);
    u32 idx = hash >> (32 - map->size_log2);
    u8 extra = hm_hash_extra(hash);
    struct hm_bucket_t *bucket = &map->buckets[idx];
    return get_from_bucket(bucket, key, key_size, extra);
}

void *hashmap_get(struct hashmap_t *map, void *key, u32 key_size)
{
    struct hm_entry_t *entry = get_entry(map, key, key_size);
    if (entry == NULL)
        return NULL;
    return entry->value;
}

static void re_insert(u32 size_log2, struct hm_bucket_t *buckets, struct hm_entry_t *entry)
{
    u32 hash = hash_func_m(entry->key, entry->key_size);
    u32 idx = hash >> (32 - size_log2);
    insert(&buckets[idx], entry);
}

static void increase(struct hashmap_t *map)
{
    // TODO: instead of moving all entries in one go, we can only move over some
    // buckets
    //       and invalidate the rest and move them once necesary

    map->size_log2++;
    assert(map->size_log2 < 32);

    int n_buckets = N_BUCKETS(map->size_log2);
    struct hm_bucket_t *new_buckets = malloc(sizeof(struct hm_bucket_t) * n_buckets);
    for (int i = 0; i < n_buckets; i++) {
        // NOTE: trenger det å være en peker?
        struct hm_bucket_t *bucket = &new_buckets[i];
        /* set all entries to NULL */
        for (u8 j = 0; j < HM_BUCKET_SIZE; j++) {
            bucket->entries[j].key = NULL;
            bucket->entries[j].alloc_flag = false;
        }
    }

    /* move all entries into the new buckets */
    int old_n_buckets = N_BUCKETS(map->size_log2 - 1);
    for (int i = 0; i < old_n_buckets; i++) {
        struct hm_bucket_t bucket = map->buckets[i];
        for (u8 j = 0; j < HM_BUCKET_SIZE; j++) {
            if (bucket.entries[j].key != NULL) {
                re_insert(map->size_log2, new_buckets, &bucket.entries[j]);
                entry_free(&bucket.entries[j]);
            }
        }
    }

    free(map->buckets);
    map->buckets = new_buckets;
}

void hashmap_put(struct hashmap_t *map, void *key, u32 key_size, void *value, u32 val_size,
                 bool alloc_flag)
{
    double load_factor = (double)map->len / (N_BUCKETS(map->size_log2) * HM_BUCKET_SIZE);

    if (load_factor >= 0.75)
        increase(map);

    u32 hash = hash_func_m(key, key_size);
    u32 idx = hash >> (32 - map->size_log2);
    u8 extra = hm_hash_extra(hash);
    struct hm_entry_t new = { key, value, key_size, val_size, extra, alloc_flag };
    int rc = insert(&map->buckets[idx], &new);

    if (rc == _HM_FULL) {
        increase(map);
        hashmap_put(map, key, key_size, value, val_size, alloc_flag);
    }

    if (rc == _HM_SUCCESS)
        map->len++;
}

void hashmap_init(struct hashmap_t *map)
{
    map->len = 0;
    map->size_log2 = HM_STARTING_BUCKETS_LOG2;

    int n_buckets = N_BUCKETS(map->size_log2);
    map->buckets = calloc(n_buckets, sizeof(struct hm_bucket_t));
    for (int i = 0; i < n_buckets; i++) {
        struct hm_bucket_t *bucket = &map->buckets[i];
        /* set all entries to NULL */
        for (u8 j = 0; j < HM_BUCKET_SIZE; j++) {
            bucket->entries[j].key = NULL;
            bucket->entries[j].alloc_flag = false;
        }
    }
}

bool hashmap_rm(struct hashmap_t *map, void *key, u32 key_size)
{
    struct hm_entry_t *entry = get_entry(map, key, key_size);
    if (entry == NULL) // || entry->value == NULL)
        return false;

    free(entry->key);
    entry->key = NULL;
    if (entry->alloc_flag) {
        free(entry->value);
        entry->alloc_flag = false;
    }

    map->len--;
    return true;
}

void hashmap_free(struct hashmap_t *map)
{
    int n_buckets = N_BUCKETS(map->size_log2);
    for (int i = 0; i < n_buckets; i++) {
        struct hm_bucket_t *bucket = &map->buckets[i];
        /* set all entries to NULL */
        for (u8 j = 0; j < HM_BUCKET_SIZE; j++) {
            struct hm_entry_t *entry = &bucket->entries[j];
            if (entry->key != NULL)
                entry_free(entry);
        }
    }

    free(map->buckets);
}

void hashmap_get_values(struct hashmap_t *map, void **return_ptr)
{
    size_t count = 0;

    for (int i = 0; i < N_BUCKETS(map->size_log2); i++) {
        struct hm_bucket_t *bucket = &map->buckets[i];
        for (int j = 0; j < HM_BUCKET_SIZE; j++) {
            struct hm_entry_t entry = bucket->entries[j];
            if (entry.key == NULL)
                continue;

            return_ptr[count++] = entry.value;
            if (count == map->len)
                return;
        }
    }
}

void hashmap_get_keys(struct hashmap_t *map, void **return_ptr)
{
    size_t count = 0;

    for (int i = 0; i < N_BUCKETS(map->size_log2); i++) {
        struct hm_bucket_t *bucket = &map->buckets[i];
        for (int j = 0; j < HM_BUCKET_SIZE; j++) {
            struct hm_entry_t entry = bucket->entries[j];
            if (entry.key == NULL)
                continue;

            return_ptr[count++] = entry.key;
            if (count == map->len)
                return;
        }
    }
}

void arraylist_init(struct arraylist_t *arr, u32 T_size)
{
#ifdef DARR_STARTING_CAP
    da->cap = DARR_STARTING_CAP;
#else
    arr->cap = 0;
    arr->cap = GROW_CAPACITY(arr->cap);
#endif

    arr->size = 0;
    arr->T_size = T_size;
    arr->data = malloc(T_size * arr->cap);
}

void arraylist_free(struct arraylist_t *arr)
{
    free(arr->data);
}

static void ensure_capacity(struct arraylist_t *arr, size_t idx)
{
    if (idx >= arr->cap) {
        /* increase capacity */
        arr->cap = GROW_CAPACITY(arr->cap);
        arr->data = nicc_internal_realloc(arr->data, arr->T_size * arr->cap);
    }
}

static void *get_element(struct arraylist_t *arr, size_t idx)
{
    uintptr_t result = (uintptr_t)arr->data + (uintptr_t)(idx * arr->T_size);
    return (void *)result;
}

bool arraylist_set(struct arraylist_t *arr, void *val, size_t idx)
{
    /* this is a bit conservative maybe, but it's not possible to set a value at a
     * position greater than the size as we don't have any mechanisms to keep
     * track of this. a bit map or something may be useful, idk.
     */
    if (idx > arr->size)
        return false;
    ensure_capacity(arr, idx);
    memcpy(get_element(arr, idx), val, arr->T_size);

    /* special case where we actually appended to the arraylist */
    if (idx == arr->size)
        arr->size++;

    return true;
}

bool arraylist_append(struct arraylist_t *arr, void *val)
{
    bool success = arraylist_set(arr, val, arr->size);
    if (!success)
        return false;
    return true;
}

void *arraylist_get(struct arraylist_t *arr, size_t idx)
{
    if (idx >= arr->size)
        return NULL;

    return get_element(arr, idx);
}

void arraylist_get_copy(struct arraylist_t *arr, size_t idx, void *return_ptr)
{
    void *element = arraylist_get(arr, idx);
    if (element == NULL) {
        return_ptr = NULL;
        return;
    }

    memcpy(return_ptr, element, arr->T_size);
}

bool arraylist_pop(struct arraylist_t *arr)
{
    if (arr->size == 0) {
        return false;
    }
    arr->size--;
    return true;
}

bool arraylist_pop_and_copy(struct arraylist_t *arr, void *return_ptr)
{
    if (arr->size == 0) {
        return_ptr = NULL;
        return false;
    }

    arraylist_get_copy(arr, arr->size - 1, return_ptr);
    arr->size--;
    return true;
}

size_t arraylist_index_of(struct arraylist_t *arr, void *val, equality_fn_t *eq)
{
    if (val == NULL)
        return NICC_NOT_FOUND;
    for (size_t i = 0; i < arr->size; i++) {
        if (eq(get_element(arr, i), val)) {
            return i;
        }
    }

    return NICC_NOT_FOUND;
}

bool arraylist_rm(struct arraylist_t *arr, size_t idx)
{
    if (idx >= arr->size)
        return false;

    for (size_t i = idx + 1; i < arr->size; i++)
        memcpy(get_element(arr, i - 1), get_element(arr, i), arr->T_size);

    arr->size--;
    return true;
}

bool arraylist_rmv(struct arraylist_t *arr, void *val, equality_fn_t *eq)
{
    if (val == NULL)
        return false;

    for (size_t i = 0; i < arr->size; i++) {
        if (eq(get_element(arr, i), val)) {
            return arraylist_rm(arr, i);
        }
    }

    return false;
}

bool arraylist_sort(struct arraylist_t *arr, compare_fn_t *cmp)
{
    if (arr->size == 0)
        return false;

    if (arr->size == 1)
        return true;

    qsort(arr->data, arr->size, arr->T_size, cmp);
    return true;
}


#define HEAPQ_STARTING_CAPACITY 32

#define heapq_left_child_idx(parent_idx) ((parent_idx << 1) + 1)
#define heapq_right_child_idx(parent_idx) ((parent_idx + 1) << 1)
#define heapq_parent_idx(child_idx) ((child_idx - 1) >> 1)

#define heapq_has_left(idx, size) (heapq_left_child_idx(idx) < size)
#define heapq_has_right(idx, size) (heapq_right_child_idx(idx) < size)


static inline void *heapq_left_child(struct heapq_t *hq, int idx)
{
    return hq->items[heapq_left_child_idx(idx)];
}

static inline void *heapq_right_child(struct heapq_t *hq, int idx)
{
    return hq->items[heapq_right_child_idx(idx)];
}

static inline void *heapq_parent(struct heapq_t *hq, int idx)
{
    return hq->items[heapq_parent_idx(idx)];
}

static void heapq_swap(struct heapq_t *hq, int a, int b)
{
    void *tmp = hq->items[a];
    hq->items[a] = hq->items[b];
    hq->items[b] = tmp;
}

static void heapify_up(struct heapq_t *hq)
{
    int idx = hq->size - 1;
    int parent_idx = heapq_parent_idx(idx);
    /* keep "repearing" heap as long as parent is greater than child */
    while (parent_idx >= 0 && hq->cmp(hq->items[parent_idx], hq->items[idx]) > 0) {
        heapq_swap(hq, parent_idx, idx);
        /* walk upwards */
        idx = heapq_parent_idx(idx);
        parent_idx = heapq_parent_idx(idx);
    }
}

static void heapify_down(struct heapq_t *hq)
{
    int idx = 0;
    int min_idx;
    while (heapq_has_left(idx, hq->size)) {
        min_idx = heapq_left_child_idx(idx);
        if (heapq_has_right(idx, hq->size) &&
            hq->cmp(hq->items[min_idx], heapq_right_child(hq, idx)) > 0)
            min_idx = heapq_right_child_idx(idx);

        if (hq->cmp(hq->items[min_idx], hq->items[idx]) > 0) {
            break;
        } else {
            heapq_swap(hq, idx, min_idx);
            idx = min_idx;
        }
    }
}

void *heapq_get(struct heapq_t *hq, int idx)
{
    if (idx < 0 || idx >= hq->size)
        return NULL;

    return hq->items[idx];
}

void *heapq_pop(struct heapq_t *hq)
{
    struct node_t *item = heapq_get(hq, 0);
    if (item == NULL)
        return NULL;

    hq->items[0] = hq->items[--hq->size];
    heapify_down(hq);
    return item;
}

void heapq_push(struct heapq_t *hq, void *item)
{
    if (hq->size >= hq->capacity) {
        hq->capacity = GROW_CAPACITY(hq->capacity);
        hq->items = GROW_ARRAY(void *, hq, hq->capacity);
    }
    hq->items[hq->size++] = item;
    heapify_up(hq);
}

void heapq_free(struct heapq_t *hq)
{
    free(hq->items);
}

void heapq_init(struct heapq_t *hq, compare_fn_t *cmp)
{
    hq->size = 0;
    hq->capacity = HEAPQ_STARTING_CAPACITY;
    hq->items = malloc(hq->capacity * sizeof(void *));
    hq->cmp = cmp;
}

static void heap_sort_internal(u8 *left, u8 *right, size_t size, compare_fn_t cmp)
{
    struct heapq_t heap;
    heapq_init(&heap, cmp);

    u8 *p = left;
    u8 elems[(right - left) / size][size];
    u32 i = 0;
    while (p <= right) {
        memcpy(*(elems + i), p, size);
        heapq_push(&heap, *(elems + i++));
        p += size;
    }

    p = right;

    while (p >= left) {
        u8 *elem = (u8 *)heapq_pop(&heap);
        BYTE_SWAP(p, elem, size);
        p -= size;
    }

    heapq_free(&heap);
}

void heap_sort(const void *base, size_t nmemb, size_t size, compare_fn_t cmp)
{
    if (!nmemb)
        return;

    u8 *base_ptr = (u8 *)base;
    u8 *left_ptr = base_ptr;
    u8 *right_ptr = base_ptr + size * (nmemb - 1);
    heap_sort_internal(left_ptr, right_ptr, size, cmp);
}


void linkedlist_init(struct linkedlist_t *ll, u32 T_size)
{
    ll->size = 0;
    ll->head = NULL;
    ll->tail = NULL;
    ll->T_size = T_size;
}

void linkedlist_free(struct linkedlist_t *ll)
{
    struct linkedlist_item_t *next = ll->head;
    while (next != NULL) {
        struct linkedlist_item_t *this = next;
        next = next->next;
        free(this);
    }
}

void linkedlist_append(struct linkedlist_t *ll, void *data)
{
    ll->size++;

    struct linkedlist_item_t *item = malloc(sizeof(struct linkedlist_item_t));
    item->data = data;
    item->prev = NULL;
    item->next = NULL;

    /* base case where ll is empty */
    if (ll->head == NULL) {
        ll->head = item;
        ll->tail = ll->head;
        return;
    }

    struct linkedlist_item_t *old_tail = ll->tail;
    item->prev = old_tail;
    old_tail->next = item;
    ll->tail = item;
}

void linkedlist_remove_item(struct linkedlist_t *ll, struct linkedlist_item_t *to_remove)
{
    ll->size--;
    struct linkedlist_item_t *prev_item = to_remove->prev;
    struct linkedlist_item_t *next_item = to_remove->next;

    if (prev_item != NULL) {
        prev_item->next = next_item;
    } else {
        /* just removed head, so need to update it */
        ll->head = next_item;
    }

    if (next_item != NULL) {
        next_item->prev = prev_item;
    } else {
        /* just removed tail, so need to update it */
        ll->tail = prev_item;
    }

    free(to_remove);
}

bool linkedlist_remove_idx(struct linkedlist_t *ll, size_t idx)
{
    if (idx > ll->size)
        return false;

    size_t i = 0;
    for (struct linkedlist_item_t *item = ll->head; item != NULL; item = item->next) {
        if (i == idx) {
            linkedlist_remove_item(ll, item);
            return true;
        }
        i++;
    }

    /* should not be reached */
    return false;
}

bool linkedlist_remove(struct linkedlist_t *ll, void *data)
{
    for (struct linkedlist_item_t *item = ll->head; item != NULL; item = item->next) {
        if (nicc_data_eq(item->data, data, ll->T_size)) {
            linkedlist_remove_item(ll, item);
            return true;
        }
    }

    return false;
}

void linkedlist_print(struct linkedlist_t *ll)
{
    for (struct linkedlist_item_t *item = ll->head; item != NULL; item = item->next) {
        printf("%p\n", item->data);
    }
}

/*
 * Originally from str.c
 */
u32 str_view_to_u32(Str8View view, bool *success)
{
    char buf[32];
    if (view.len >= sizeof(buf)) {
        if (success != NULL) {
            *success = false;
        }
        return 0;
    }

    memcpy(buf, view.str, view.len);
    buf[view.len] = 0;

    char *endptr;
    unsigned long result = strtoul(buf, &endptr, 10);

    /* Check for conversion errors */
    if (endptr == buf || *endptr != 0 || result > UINT32_MAX) {
        if (success) {
            *success = false;
        }
        return 0;
    }

    if (success) {
        *success = true;
    }
    return (u32)result;
}

Str8Builder make_str_builder(Arena *arena)
{
    Str8Builder sb = {
        .arena = arena,
        .str = (Str8){ .len = 0 },
        .cap = 16,
    };

    sb.str.str = m_arena_alloc(arena, sb.cap);
    return sb;
}

void str_builder_append_u8(Str8Builder *sb, u8 c)
{
    if (sb->str.len == sb->cap) {
        /* Double the allocation */
        (void)m_arena_alloc(sb->arena, sb->cap);
        sb->cap *= 2;
    }

    sb->str.str[sb->str.len++] = c;
}

void str_builder_append_cstr(Str8Builder *sb, char *cstr, u32 len)
{
    while (sb->str.len + len > sb->cap) {
        /* Double the allocation */
        (void)m_arena_alloc(sb->arena, sb->cap);
        sb->cap *= 2;
    }

    memcpy((char *)(sb->str.str + sb->str.len), cstr, len);
    sb->str.len += len;
}


void str_builder_append_str8(Str8Builder *sb, Str8 str)
{
    str_builder_append_cstr(sb, (char *)str.str, str.len);
}

void str_builder_sprintf(Str8Builder *sb, char *fmt, int count, ...)
{
    va_list args;
    va_start(args, count);

    for (char *p = fmt; *p != '\0'; p++) {
        if (*p == '%' && *(p + 1) != '\0') {
            p++; // Skip the '%' character
            switch (*p) {
            /* Integer */
            case 'd': {
                int num = va_arg(args, int);
                char num_buffer[32];
                int num_len = snprintf(num_buffer, sizeof(num_buffer), "%d", num);

                for (int i = 0; i < num_len; i++) {
                    str_builder_append_u8(sb, (u8)num_buffer[i]);
                }
            } break;
            /* String */
            case 's': {
                char *str = va_arg(args, char *);
                size_t str_len = strlen(str);
                str_builder_append_cstr(sb, str, str_len);
                break;
            }
            default:
                /* If unknown specifier, append the '%' and the following character */
                str_builder_append_u8(sb, (u8)'%');
                str_builder_append_u8(sb, (u8)*p);
                break;
            }
        } else {
            /* Regular char */
            str_builder_append_u8(sb, (u8)*p);
        }
    }

    va_end(args);
}

Str8 str_builder_end(Str8Builder *sb, bool add_null_terminator)
{
    if (add_null_terminator) {
        str_builder_append_u8(sb, 0);
    }
    Str8 final = sb->str;
    assert(final.str[final.len - 1] == 0);
    assert(final.len != 0);
    final.len -= 1;
    return final;
}

void str_list_init(Str8List *list)
{
    list->len = 0;
    list->cap = 16;
    list->strs = malloc(sizeof(Str8) * list->cap);
}

void str_list_free(Str8List *list)
{
    free(list->strs);
}

u32 str_list_push(Str8List *list, Str8 str)
{
    if (list->len >= list->cap) {
        list->cap *= 2;
        list->strs = realloc(list->strs, sizeof(Str8) * list->cap);
    }
    list->strs[list->len] = str;
    list->len++;
    return list->len - 1;
}

u32 str_list_push_cstr(Arena *arena, Str8List *list, char *cstr)
{
    size_t len = strlen(cstr);
    u8 *str = m_arena_alloc(arena, len + 1);
    memcpy(str, cstr, len);
    str[len] = 0;
    return str_list_push(list, (Str8){ .len = len, .str = str });
}

void str_list_print(Str8List *list)
{
    for (u32 i = 0; i < list->len; i++) {
        printf("[%d] %s\n", i, list->strs[i].str);
    }
}

Str8List str_list_from_split(Str8 input, char delim)
{
    Str8List list;
    str_list_init(&list);

    size_t start = 0;
    /* Loop including null terminator */
    for (size_t i = 0; i <= input.len; i++) {
        if (i == input.len || input.str[i] == delim) {
            size_t length = i - start;
            /* Avoid pushing empty string */
            if (length > 0) {
                Str8 str = { .len = length, .str = input.str + start };
                str_list_push(&list, str);
            }
            start = i + 1;
        }
    }
    return list;
}

/*
 * Originally from log.c
 */
Logger global_logger = { LOG_WARN };

void log_init_global(LogLevel log_level)
{
    global_logger.log_level = log_level;
}

static void log_msg(FILE *out, char *prefix, char *fmt, va_list args)
{
    fprintf(out, "[%s] ", prefix);
    vfprintf(out, fmt, args);
    fprintf(out, "\n");
}

void log_bad_event(LogLevel level, char *prefix, char *fmt, ...)
{
    if (global_logger.log_level > level) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    log_msg(stderr, prefix, fmt, args);
    va_end(args);
}

void log_debug(char *file, size_t line, bool is_a_fixme, char *fmt, ...)
{
    if (global_logger.log_level != LOG_ALL) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    fprintf(stdout, "[%s] %s@%zu | ", is_a_fixme ? "FIXME" : "DEBUG", file, line);
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
    va_end(args);
}

void log_debug_str8(Str8 msg, char *file, size_t line, bool is_a_fixme)
{
    if (global_logger.log_level != LOG_ALL) {
        return;
    }
    printf("[%s] %s:%zu | %s\n", is_a_fixme ? "FIXME" : "DEBUG", file, line, msg.str);
}

/*
 * Originally from nag.c
 */
typedef NAG_Order (*GraphTraverse)(NAG_Graph *graph, NAG_Idx start_node, u8 *visited);


NAG_Graph nag_make_graph(Arena *persist, Arena *scratch, NAG_Idx n_nodes)
{
    NAG_Graph graph = {
        .n_nodes = n_nodes,
        .scratch_arena = scratch,
        .persist_arena = persist,
        .neighbor_list = m_arena_alloc(persist, sizeof(NAG_GraphNode) * n_nodes),
    };
    return graph;
}

void nag_add_edge(NAG_Graph *graph, NAG_Idx from, NAG_Idx to)
{
    assert(from <= graph->n_nodes);
    // NOTE: This is the most naive way we can add eges and probably quite poor for performance
    //       I will improve this if/when it becomes noticable.
    NAG_GraphNode *first = graph->neighbor_list[from];
    NAG_GraphNode *new_node = m_arena_alloc(graph->persist_arena, sizeof(NAG_GraphNode));
    new_node->id = to;
    new_node->next = first;
    graph->neighbor_list[from] = new_node;
}

void nag_print(NAG_Graph *graph)
{
    for (NAG_Idx i = 0; i < graph->n_nodes; i++) {
        printf("[%d] -> ", i);
        NAG_GraphNode *node = graph->neighbor_list[i];
        while (node != NULL) {
            printf("%d, ", node->id);
            node = node->next;
        }
        putchar('\n');
    }
}

static NAG_OrderList nag_traverse_all(NAG_Graph *graph, GraphTraverse traverse_func)
{
    u8 *visited = m_arena_alloc(graph->scratch_arena, sizeof(bool) * graph->n_nodes);
    memset(visited, false, sizeof(NAG_Idx) * graph->n_nodes);

    NAG_OrderList result = { 0 };
    u32 n_orders_allocated = 8;
    result.orders = malloc(sizeof(NAG_Order) * n_orders_allocated);

    for (NAG_Idx i = 0; i < graph->n_nodes; i++) {
        if (visited[i]) {
            continue;
        }
        NAG_Order order = traverse_func(graph, i, visited);
        result.orders[result.n++] = order;
        if (result.n == n_orders_allocated) {
            n_orders_allocated += 8;
            result.orders = realloc(result.orders, sizeof(NAG_Order) * n_orders_allocated);
        }
    }

    m_arena_clear(graph->scratch_arena);
    return result;
}

static inline bool linear_alloc_nodes(Arena *arena, u32 n)
{
    void *r = m_arena_alloc_internal(arena, sizeof(NAG_Idx) * n, sizeof(NAG_Idx), false);
    return r != NULL;
}

static NAG_Order nag_dfs_internal(NAG_Graph *graph, NAG_Idx start_node, u8 *visited)
{
    /* This will grow linearly on the persist arena as we add nodes to the order */
    NAG_Idx *ordered = m_arena_alloc(graph->persist_arena, sizeof(NAG_Idx) * 1);
    NAG_Idx ordered_len = 0;

    /* Everything we allocate on the scratch arena will be released before we returned */
    ArenaTmp tmp_arena = m_arena_tmp_init(graph->scratch_arena);
    NAG_Idx stack_size = NAG_STACK_GROW_SIZE;
    NAG_Idx stack_top = 1;
    /* Similar to ordered. Will grow linearly on the scratch arena as we add nodes to the stack */
    NAG_Idx *stack = m_arena_alloc_internal(graph->scratch_arena, sizeof(NAG_Idx) * stack_size,
                                            sizeof(NAG_Idx), false);
    stack[0] = start_node;

    while (stack_top != 0) {
        NAG_Idx current_node = stack[--stack_top];
        if (visited[current_node]) {
            continue;
        }
        visited[current_node] = true;
        ordered[ordered_len++] = current_node;
        if (!linear_alloc_nodes(graph->persist_arena, 1)) {
            /* Persist arena is full. Report error. */
        }

        for (NAG_GraphNode *n = graph->neighbor_list[current_node]; n != NULL; n = n->next) {
            stack[stack_top++] = n->id;
            if (stack_top == stack_size) {
                if (!linear_alloc_nodes(graph->scratch_arena, NAG_STACK_GROW_SIZE)) {
                    /* Scratch arena is full. Report error. */
                }
                stack_size += NAG_STACK_GROW_SIZE;
            }
        }
    }
    m_arena_tmp_release(tmp_arena); // Reclaims the memory to the arena, not to the OS
    return (NAG_Order){ .n_nodes = ordered_len, .nodes = ordered };
}

NAG_Order nag_dfs_from(NAG_Graph *graph, NAG_Idx start_node)
{
    u8 *visited = m_arena_alloc(graph->scratch_arena, sizeof(bool) * graph->n_nodes);
    memset(visited, false, sizeof(NAG_Idx) * graph->n_nodes);
    NAG_Order dfs_order = nag_dfs_internal(graph, start_node, visited);
    m_arena_clear(graph->scratch_arena);
    return dfs_order;
}

NAG_OrderList nag_dfs(NAG_Graph *graph)
{
    return nag_traverse_all(graph, nag_dfs_internal);
}

static NAG_Order nag_bfs_internal(NAG_Graph *graph, NAG_Idx start_node, u8 *visited)
{
    /* This will grow linearly on the persist arena as we add nodes to the order */
    NAG_Idx *ordered = m_arena_alloc(graph->persist_arena, sizeof(NAG_Idx) * 1);
    NAG_Idx ordered_len = 0;

    /* Everything we allocate on the scratch arena will be released before we returned */
    ArenaTmp tmp_arena = m_arena_tmp_init(graph->scratch_arena);

    NAG_Idx queue_size = NAG_QUEUE_GROW_SIZE;
    NAG_Idx queue_low = 0;
    NAG_Idx queue_high = 1;
    /* Similar to ordered. Will grow linearly on the scratch arena if we need to increase the size
     */
    NAG_Idx *queue = m_arena_alloc_internal(
        graph->scratch_arena, sizeof(NAG_Idx) * NAG_QUEUE_GROW_SIZE, sizeof(NAG_Idx), false);
    queue[0] = start_node;

    while (queue_low != queue_high) {
        NAG_Idx current_node = queue[queue_low++];
        if (visited[current_node]) {
            continue;
        }
        visited[current_node] = true;
        ordered[ordered_len++] = current_node;
        if (!linear_alloc_nodes(graph->persist_arena, 1)) {
            /* Persist arena is full. Report error. */
        }

        for (NAG_GraphNode *n = graph->neighbor_list[current_node]; n != NULL; n = n->next) {
            queue[queue_high++] = n->id;
            /*
             * Queue is full.
             * If we have a lot of unused space to the left, we shift the entire queue
             * downards. If not, we increase the allocation.
             */
            if (queue_high == queue_size) {
                /* Shift left */
                if (queue_low > queue_size / 2) {
                    memmove(queue, queue + queue_low, queue_high - queue_low + 1);
                    queue_high -= queue_low;
                    queue_low = 0;
                }
                /* Increase the allocation for the queue */
                if (!linear_alloc_nodes(graph->scratch_arena, NAG_QUEUE_GROW_SIZE)) {
                    /* Scratch arena is full. Report error. */
                }
                queue_size += NAG_QUEUE_GROW_SIZE;
            }
        }
    }
    m_arena_tmp_release(tmp_arena); // Reclaims the memory to the arena, not to the OS
    return (NAG_Order){ .n_nodes = ordered_len, .nodes = ordered };
}

NAG_Order nag_bfs_from(NAG_Graph *graph, NAG_Idx start_node)
{
    u8 *visited = m_arena_alloc(graph->scratch_arena, sizeof(bool) * graph->n_nodes);
    memset(visited, false, sizeof(NAG_Idx) * graph->n_nodes);
    NAG_Order bfs_order = nag_bfs_internal(graph, start_node, visited);
    m_arena_clear(graph->scratch_arena);
    return bfs_order;
}

NAG_OrderList nag_bfs(NAG_Graph *graph)
{
    return nag_traverse_all(graph, nag_bfs_internal);
}

static NAG_Order nag_toposort_from_internal(NAG_Graph *graph, NAG_Idx start_node, u8 *visited)
{
    /* This will grow linearly on the persist arena as we add nodes to the order */
    NAG_Idx *ordered = m_arena_alloc(graph->persist_arena, sizeof(NAG_Idx) * 1);
    NAG_Idx ordered_len = 0;

    /* NOTE: Most of the code below here is just DFS + some backtracking */

    /* Everything we allocate on the scratch arena will be released before we returned */
    ArenaTmp tmp_arena = m_arena_tmp_init(graph->scratch_arena);
    NAG_Idx stack_size = NAG_STACK_GROW_SIZE;
    NAG_Idx stack_top = 1;
    /* Similar to ordered. Will grow linearly on the scratch arena as we add nodes to the stack */
    NAG_Idx *stack = m_arena_alloc_internal(graph->scratch_arena, sizeof(NAG_Idx) * stack_size,
                                            sizeof(NAG_Idx), false);
    stack[0] = start_node;

    while (stack_top != 0) {
        NAG_Idx current_node = stack[--stack_top];
        if (visited[current_node]) {
            ordered[ordered_len++] = current_node;
            if (!linear_alloc_nodes(graph->persist_arena, 1)) {
                /* Persist arena is full. Report error. */
            }
            continue;
        }

        visited[current_node] = true;
        stack[stack_top++] =
            current_node; /* Next time we pop this node all neighbours have been visited */
        if (stack_top == stack_size) {
            if (!linear_alloc_nodes(graph->scratch_arena, NAG_STACK_GROW_SIZE)) {
                /* Scratch arena is full. Report error. */
            }
            stack_size += NAG_STACK_GROW_SIZE;
        }

        for (NAG_GraphNode *n = graph->neighbor_list[current_node]; n != NULL; n = n->next) {
            stack[stack_top++] = n->id;
            if (stack_top == stack_size) {
                if (!linear_alloc_nodes(graph->scratch_arena, NAG_STACK_GROW_SIZE)) {
                    /* Scratch arena is full. Report error. */
                }
                stack_size += NAG_STACK_GROW_SIZE;
            }
        }
    }
    m_arena_tmp_release(tmp_arena); /* Reclaims the memory to the arena, not to the OS */
    return (NAG_Order){ .n_nodes = ordered_len, .nodes = ordered };
}

NAG_Order nag_rev_toposort(NAG_Graph *graph)
{
    NAG_OrderList all = nag_traverse_all(graph, nag_toposort_from_internal);
    bool *included = m_arena_alloc(graph->scratch_arena, sizeof(NAG_Idx) * graph->n_nodes);
    memset(included, 0, sizeof(NAG_Idx) * graph->n_nodes);

    NAG_Order final;
    final.n_nodes = 0;
    final.nodes = m_arena_alloc(graph->persist_arena, sizeof(NAG_Idx) * graph->n_nodes);

    for (u32 i = 0; i < all.n; i++) {
        NAG_Order current = all.orders[i];
        for (u32 j = 0; j < current.n_nodes; j++) {
            NAG_Idx current_idx = current.nodes[j];
            if (!included[current_idx]) {
                included[current_idx] = true;
                final.nodes[final.n_nodes++] = current_idx;
            }
        }
    }

    return final;
}

typedef struct {
    NAG_Idx *stack;
    bool *on_stack;
    NAG_Idx *low_link;
    NAG_Idx *discovery_time;
    NAG_Idx time;
    NAG_Idx stack_top;
    Arena *scratch_arena;
} NAG_TarjanContext;

static void nag_tarjan_scc_dfs(NAG_Graph *graph, NAG_Idx node, NAG_TarjanContext *ctx,
                               NAG_OrderList *sccs)
{
    ctx->discovery_time[node] = ctx->time;
    ctx->low_link[node] = ctx->time;
    ctx->time++;
    ctx->stack[ctx->stack_top++] = node;
    ctx->on_stack[node] = true;

    for (NAG_GraphNode *neighbor = graph->neighbor_list[node]; neighbor != NULL;
         neighbor = neighbor->next) {
        NAG_Idx neighbor_id = neighbor->id;
        if (ctx->discovery_time[neighbor_id] == NAG_UNDISCOVERED) {
            /* If neighbor is not yet visited, recurse on it */
            nag_tarjan_scc_dfs(graph, neighbor_id, ctx, sccs);
            ctx->low_link[node] = NAG_MIN(ctx->low_link[node], ctx->low_link[neighbor_id]);
        } else if (ctx->on_stack[neighbor_id]) {
            /* Update low-link value if the neighbor is on the stack */
            ctx->low_link[node] = NAG_MIN(ctx->low_link[node], ctx->discovery_time[neighbor_id]);
        }
    }

    /* If node is a root node, pop the stack and form an SCC */
    if (ctx->low_link[node] == ctx->discovery_time[node]) {
        NAG_Order scc = { 0 };
        /* This will grow linearly on the persist arena as we add nodes to the order */
        scc.nodes = m_arena_alloc(graph->persist_arena, sizeof(NAG_Idx) * 1);

        while (1) {
            NAG_Idx top = ctx->stack[--ctx->stack_top];
            ctx->on_stack[top] = false;
            scc.nodes[scc.n_nodes++] = top;
            if (!linear_alloc_nodes(graph->persist_arena, 1)) {
                /* Persist arena is full. Report error. */
            }
            if (top == node)
                break;
        }

        /* This implemention does not care about trivial scc's */
        if (scc.n_nodes != 1) {
            if (sccs->n == 0 || sccs->n % 8 == 0) { // TODO: this is hacky
                sccs->orders = realloc(sccs->orders, sizeof(NAG_Order) * (sccs->n + 8));
            }
            sccs->orders[sccs->n++] = scc;
        }
    }
}

NAG_OrderList nag_scc(NAG_Graph *graph)
{
    NAG_OrderList sccs;
    sccs.n = 0;
    sccs.orders = malloc(sizeof(NAG_Order) * sccs.n);

    NAG_TarjanContext ctx;
    ctx.stack = m_arena_alloc(graph->scratch_arena, sizeof(NAG_Idx) * graph->n_nodes);
    ctx.on_stack = m_arena_alloc(graph->scratch_arena, sizeof(bool) * graph->n_nodes);
    ctx.low_link = m_arena_alloc(graph->scratch_arena, sizeof(NAG_Idx) * graph->n_nodes);
    ctx.discovery_time = m_arena_alloc(graph->scratch_arena, sizeof(NAG_Idx) * graph->n_nodes);
    ctx.time = 0;
    ctx.stack_top = 0;
    ctx.scratch_arena = graph->scratch_arena;

    memset(ctx.on_stack, false, sizeof(bool) * graph->n_nodes);
    memset(ctx.discovery_time, NAG_UNDISCOVERED, sizeof(NAG_Idx) * graph->n_nodes);

    for (NAG_Idx i = 0; i < graph->n_nodes; i++) {
        if (ctx.discovery_time[i] == NAG_UNDISCOVERED) {
            nag_tarjan_scc_dfs(graph, i, &ctx, &sccs);
        }
    }

    m_arena_clear(graph->scratch_arena);
    return sccs;
}

/*
 * Originally from threadpool.c
 */
static bool task_queue_empty(TP_TaskQueue *task_queue)
{
    /* NOTE: Assumes the task_queue lock is acquired ! */
    return task_queue->head == task_queue->tail;
}

static TP_Task task_queue_next(TP_TaskQueue *task_queue)
{
    u32 head = task_queue->head;
    task_queue->head++;
    if (task_queue->head >= task_queue->size) {
        task_queue->head = 0;
    }
    return task_queue->tasks[head];
}

static void task_info_map_insert(TP_TaskInfoMap *map, TP_TaskHandle handle, TP_TaskInfo task_info);

static void task_info_map_resize(TP_TaskInfoMap *map, u32 new_capacity)
{
    /* NOTE: Assumes the task_queue lock is acquired ! */
    TP_TaskHandle *old_keys = map->keys;
    TP_TaskInfo *old_values = map->values;
    u32 old_capacity = map->slots_allocated;

    map->slots_allocated = new_capacity;
    map->keys = calloc(new_capacity, sizeof(TP_TaskHandle));
    map->values = malloc(new_capacity * sizeof(TP_TaskInfo));

    /* Re-insert the entries */
    map->slots_filled = 0;
    for (u32 i = 0; i < old_capacity; i++) {
        if (old_keys[i] != TASK_HANDLE_UNUSED) {
            task_info_map_insert(map, old_keys[i], old_values[i]);
        }
    }

    free(old_keys);
    free(old_values);
}

static void task_info_map_insert(TP_TaskInfoMap *map, TP_TaskHandle handle, TP_TaskInfo task_info)
{
    /* NOTE: Assumes the task_queue lock is acquired ! */
    if ((map->slots_filled + 1) * 100 >= map->slots_allocated * MAP_LOAD_FACTOR_PERCENT) {
        task_info_map_resize(map, map->slots_allocated * 2);
    }

    u32 index = handle % map->slots_allocated;
    while (map->keys[index] != TASK_HANDLE_UNUSED) {
        // TODO: if we ensure this is a power of two we can make this fast and remove the modulo
        index = (index + 1) % map->slots_allocated;
    }

    /* Insert the entry at the index */
    map->keys[index] = handle;
    map->values[index] = task_info;
    map->slots_filled++;
}

static bool task_info_map_update(TP_TaskInfoMap *map, TP_TaskHandle handle, TP_TaskInfo task_info)
{
    /* NOTE: Assumes the task_queue lock is acquired ! */
    u32 index = handle % map->slots_allocated;
    u32 start_index = index;

    while (map->keys[index] != handle) {
        index = (index + 1) % map->slots_allocated;
        if (index == start_index) {
            return false;
        }
    }

    map->values[index] = task_info;
    return true;
}

static TP_TaskInfo task_info_map_get(TP_TaskInfoMap *map, TP_TaskHandle handle,
                                     bool remove_if_finished)
{
    /* NOTE: Assumes the task_queue lock is acquired ! */
    u32 index = handle % map->slots_allocated;
    u32 start_index = index;

    while (map->keys[index] != handle) {
        index = (index + 1) % map->slots_allocated;
        if (index == start_index) {
            /*
             * We completed a loop of the map without finding the handle.
             * In practice, this should never happen since trying to fetch the info of a hanlde
             * without a task info object should never really happen.
             */
            return (TP_TaskInfo){ .status = TASK_NOT_FOUND };
        }
    }

    TP_TaskInfo found = map->values[index];
    if (remove_if_finished && found.status == TASK_FINISHED) {
        map->keys[index] = TASK_HANDLE_UNUSED;
        map->slots_filled--;
    }
    return found;
}

static void *tp_worker_loop(void *arg)
{
    TP_ThreadPool *tp = (TP_ThreadPool *)arg;
    while (1) {
        pthread_mutex_lock(&tp->task_queue.lock);
        while (task_queue_empty(&tp->task_queue) && !tp->stop_new) {
            /* Only join when the queue is exhausted */
            if (tp->join) {
                pthread_mutex_unlock(&tp->task_queue.lock);
                return NULL;
            }
            pthread_cond_wait(&tp->task_queue.cv, &tp->task_queue.lock);
        }
        /* Do not run the new task */
        if (tp->stop_new) {
            pthread_mutex_unlock(&tp->task_queue.lock);
            return NULL;
        }
        TP_Task task = task_queue_next(&tp->task_queue);
        if (task.handle != TASK_HANDLE_UNUSED) {
            TP_TaskInfo task_info = { .status = TASK_RUNNING, .task = task, .result = NULL };
            task_info_map_update(&tp->task_info_map, task.handle, task_info);
        }

        pthread_cond_signal(&tp->task_queue.cv);
        pthread_mutex_unlock(&tp->task_queue.lock);

        void *result = task.func(task.args);
        if (task.handle != TASK_HANDLE_UNUSED) {
            pthread_mutex_lock(&tp->task_queue.lock);
            TP_TaskInfo task_info = { .status = TASK_FINISHED, .task = task, .result = result };
            task_info_map_update(&tp->task_info_map, task.handle, task_info);
            pthread_cond_signal(&tp->task_queue.cv);
            pthread_mutex_unlock(&tp->task_queue.lock);
        }
    }
    return NULL;
}

void tp_init(TP_ThreadPool *tp, u32 n_threads, u32 queue_size)
{
    tp->n_threads = n_threads;
    tp->threads = malloc(sizeof(pthread_t) * n_threads);

    tp->task_queue.tasks = malloc(sizeof(TP_Task) * queue_size);
    tp->task_queue.size = queue_size;
    tp->task_queue.head = 0;
    tp->task_queue.tail = 0;

    pthread_mutex_init(&tp->task_queue.lock, NULL);
    pthread_cond_init(&tp->task_queue.cv, NULL);

    tp->task_handle_counter = 0;
    tp->task_info_map.slots_filled = 0;
    tp->task_info_map.slots_allocated = 16;
    /* Zero allocate so the keys always start with the value of 0 aka TASK_HANDLE_UNUSED */
    tp->task_info_map.keys = calloc(tp->task_info_map.slots_allocated, sizeof(TP_TaskHandle));
    tp->task_info_map.values = malloc(sizeof(TP_TaskInfo) * tp->task_info_map.slots_allocated);

    tp->join = false;
    tp->stop_new = false;
}

void tp_destroy(TP_ThreadPool *tp)
{
    pthread_mutex_destroy(&tp->task_queue.lock);
    pthread_cond_destroy(&tp->task_queue.cv);
    free(tp->threads);
    free(tp->task_info_map.keys);
    free(tp->task_info_map.values);
}

void tp_start(TP_ThreadPool *tp)
{
    for (u32 i = 0; i < tp->n_threads; i++) {
        pthread_create(&tp->threads[i], NULL, tp_worker_loop, (void *)tp);
    }
}

void tp_join(TP_ThreadPool *tp)
{
    pthread_mutex_lock(&tp->task_queue.lock);
    tp->join = true;
    pthread_cond_broadcast(&tp->task_queue.cv);
    pthread_mutex_unlock(&tp->task_queue.lock);

    for (u32 i = 0; i < tp->n_threads; i++) {
        pthread_join(tp->threads[i], NULL);
    }
}

void tp_stop_new(TP_ThreadPool *tp)
{
    pthread_mutex_lock(&tp->task_queue.lock);
    tp->stop_new = true;
    pthread_cond_broadcast(&tp->task_queue.cv);
    pthread_mutex_unlock(&tp->task_queue.lock);

    for (u32 i = 0; i < tp->n_threads; i++) {
        pthread_join(tp->threads[i], NULL);
    }
}

TP_TaskHandle tp_queue_task(TP_ThreadPool *tp, TP_Function f, void *args, bool wait_if_full,
                            bool set_up_handle)
{
    pthread_mutex_lock(&tp->task_queue.lock);

    u32 tail;
    u32 next_tail;
    while (1) {
        tail = tp->task_queue.tail;
        next_tail = tp->task_queue.tail + 1;
        /* Wrap the ring buffer */
        if (next_tail >= tp->task_queue.size) {
            next_tail = 0;
        }
        if (next_tail == tp->task_queue.head) {
            /* Queue is full ! */
            if (!wait_if_full) {
                pthread_mutex_unlock(&tp->task_queue.lock);
                return TASK_HANDLE_FULL;
            }
            /* Wait for space in queue */
            pthread_cond_wait(&tp->task_queue.cv, &tp->task_queue.lock);
            continue;
        } else {
            /* Queue is not full */
            break;
        }
    }

    tp->task_queue.tail = next_tail;

    TP_Task task = { .handle = TASK_HANDLE_UNUSED, .func = f, .args = args };
    if (set_up_handle) {
        tp->task_handle_counter++;
        task.handle = tp->task_handle_counter;

        TP_TaskInfo task_info = { .status = TASK_IN_QUEUE, .task = task, .result = NULL };
        task_info_map_insert(&tp->task_info_map, task.handle, task_info);
    }

    tp->task_queue.tasks[tail] = task;

    pthread_cond_signal(&tp->task_queue.cv);
    pthread_mutex_unlock(&tp->task_queue.lock);

    return task.handle;
}

TP_TaskInfo tp_get_info(TP_ThreadPool *tp, TP_TaskHandle handle, bool remove_if_finished)
{
    pthread_mutex_lock(&tp->task_queue.lock);
    TP_TaskInfo task_info = task_info_map_get(&tp->task_info_map, handle, remove_if_finished);
    pthread_mutex_unlock(&tp->task_queue.lock);
    return task_info;
}


#endif /* BASE_IMPLEMENTATION */
