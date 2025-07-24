/*

nate_Task

depends on: nate_DynMemory

Do:
    #define NATE_TASK_IMPLEMENTATION
before you include this header in only one c / c++ compilation unit

Also you can do (option 1):
    #define NATE_TASK_NO_THREAD
to not use threads (also no mutex). It will call the tasks one by one on nate_Task_Update(),
which might make it work more robust on the web.

Also you can do (option 2):
    #define NATE_TASK_SDL3_THREAD
to use SDL for Thread and Mutex handling.

An implementation without SDL but with threads is not yet implemented (option 3).

CC0 License

by Nathanael Krauße

*/

#ifndef _NATE_TASK_HEADER
#define _NATE_TASK_HEADER

#include "nate_DynMemory.h"

typedef void(*nate_Task)(void* userdata);
extern bool nate_Task_Init();
extern bool nate_Task_Add(nate_Task function, void* userdata);
extern void nate_Task_Update(); // Start/Join/Manage Threads and Tasks
extern void nate_Task_Deinit();

//------------------------- Impl

#ifdef NATE_TASK_IMPLEMENTATION

// -------------- the part about collecting tasks
// 
static ArrayBuffer s_Tasks = ArrayBuffer0;
#define s_TasksData ((nate_OneTask*)(s_Tasks.data))
typedef struct nate_OneTask {
    nate_Task function;
    void* userdata;
} nate_OneTask;

static bool s_add_task_sync(nate_Task function, void* userdata)
{
    if (s_Tasks.size >= s_Tasks.allocated) {
        int new_size = s_Tasks.size;
        if (new_size)
            new_size <<= 1;
        else
            new_size = 8;
        if (!nate_ArrayBuffer_Alloc(&s_Tasks, sizeof(nate_OneTask), new_size)) {
            return false;
        }
    }
    
    s_TasksData[s_Tasks.size] = (nate_OneTask){
        .function = function,
        .userdata = userdata
    };
    s_Tasks.size++;
    return true;
}

static nate_OneTask s_fetch_task_sync()
{
    nate_OneTask res = {0};
    if (s_Tasks.size > 0) {
        res = s_TasksData[0];
        s_Tasks.size--;
        for (size_t i = 0; i < s_Tasks.size; i++) {
            s_TasksData[i] = s_TasksData[i+1];
        }
    }
    return res;
}

#ifdef NATE_TASK_NO_THREAD

bool nate_Task_Init(){ return true; }
void nate_Task_Deinit(){}

bool nate_Task_Add(nate_Task function, void* userdata)
{
    return s_add_task_sync(function, userdata);
}

// Update will process one Task at a time
void nate_Task_Update() // Start/Join/Manage Threads and Tasks
{
    nate_OneTask curr_task = s_fetch_task_sync();
    if (curr_task.function) {
        curr_task.function(curr_task.userdata);
    }    
}

#else // NOT NATE_TASK_NO_THREAD

#ifdef NATE_TASK_SDL3_THREAD

#include <SDL3/SDL_thread.h>
#include <SDL3/SDL_mutex.h>
#include <SDL3/SDL_log.h>

static SDL_Mutex* s_mutex = NULL;
static void s_CleanMutex()
{
    if (s_mutex) {
        SDL_DestroyMutex(s_mutex);
        s_mutex = NULL;
    }
}

static SDL_Thread* s_thread = NULL;
static void s_CleanThread()
{
    if (s_thread) {
        SDL_WaitThread(s_thread, NULL);
        s_thread = NULL;
    }
}

static bool s_EnsureMutex()
{
    if (!s_mutex)
    {
        s_mutex = SDL_CreateMutex();
        if (!s_mutex) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                "SDL_CreateMutex(...) failed:\n%s\n", SDL_GetError());
        }
    }
    return s_mutex != NULL;
}

static void s_JoinJoinableThread()
{
    if (s_thread) {
        SDL_ThreadState state = SDL_GetThreadState(s_thread);
        if (state == SDL_THREAD_COMPLETE) {
            SDL_WaitThread(s_thread, NULL);
            s_thread = NULL;
        }
    }
}

static int s_worker_function(void* _)
{
    nate_OneTask curr_task;
    do
    {
        SDL_LockMutex(s_mutex);
        curr_task = s_fetch_task_sync();
        SDL_UnlockMutex(s_mutex);
        
        if (curr_task.function)
            curr_task.function(curr_task.userdata);
    }
    while(curr_task.function);
    return 0;
}

// Please first init Mutex
static bool s_EnsureRunningThread()
{
    s_JoinJoinableThread();
    if (!s_thread) {
        s_thread = SDL_CreateThread(s_worker_function, "Task", NULL);
        if (!s_thread) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                "SDL_CreateThread(...) failed:\n%s\n", SDL_GetError());
            s_CleanMutex();
        }
    }
    return s_thread != NULL;
}

bool nate_Task_Init()
{
    return s_EnsureMutex();
}

void nate_Task_Deinit()
{
    s_CleanMutex();
    s_CleanThread();
}

bool nate_Task_Add(nate_Task function, void* userdata)
{
    SDL_LockMutex(s_mutex);
    bool res = s_add_task_sync(function, userdata);
    SDL_UnlockMutex(s_mutex);
    return res;
}

void nate_Task_Update() // Start/Join/Manage Threads and Tasks
{
    SDL_LockMutex(s_mutex);
    if (s_Tasks.size) {
        s_EnsureRunningThread();
    }
    else {
        s_JoinJoinableThread();
        if (s_Tasks.allocated) {
            nate_ArrayBuffer_Free(&s_Tasks);
        }
    }
    SDL_UnlockMutex(s_mutex);
}

#else // NOT NATE_TASK_SDL3_THREAD and NOT NATE_TASK_NO_THREAD

// TODO
#error Tasks (without SDL3) is not implemented

#endif // (NOT) NATE_TASK_SDL3_THREAD

#endif // (NOT) NATE_TASK_NO_THREAD

#endif // NATE_TASK_IMPLEMENTATION

#endif // _NATE_TASK_HEADER
