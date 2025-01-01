#include <pthread.h>
#include <stdio.h>
#include <iostream>
#include "string.h"
#include <stdarg.h>
#include <sys/time.h>
#include "semaphore.h"
#include <exception>


using namespace std;


class sem {

public:
    sem();
    sem(int num);
    ~sem();
    bool wait();
    bool post();

private:
    sem_t m_sem;


};


class locker {
public:
    // 构造函数，初始化互斥锁
    locker();

    // 析构函数，销毁互斥锁
    ~locker();

    // 加锁操作
    bool lock();

    // 解锁操作
    bool unlock();

    // 获取互斥锁对象的指针
    pthread_mutex_t* get();

private:
    pthread_mutex_t m_mutex;  // 互斥锁
};



class cond{

public:
    cond();
    ~cond();
    bool wait(pthread_mutex_t* m_mutex);
    bool timewait(pthread_mutex_t* m_mutex,struct timespec t);
    bool signal();
    bool broadcast();

private:
    pthread_cond_t m_cond;

};




template <class T>
class Blockqueue {
public:

    Blockqueue(int queue_max_size);
    ~Blockqueue();
    bool push(const T& item);
    bool pop(T& item);
    bool pop();
    bool front(T& item);
    bool back(T& item);

    // 实现的没有问题,返回的是进入函数时，容器内部是满还是不满
    bool full();

    // 实现的没有问题
    bool empty();


    //不安全的方法
    int size();


private:
    T* m_array;
    int m_max_size;
    int m_front;
    int m_back;
    int m_size;

    locker m_locker;
    cond m_cond_Cons;
    cond m_cond_Prod;

};

































class Log{

public:

    static Log* instanse();
    static void *write_log_thread(void *args);
    bool init(const char* filename, int log_buf_size, int split_lines, int max_queue_size);
    void log_write(int level, const char* format, ...);
    void flush(void);



private:
    Log();
    ~Log();

    void asyn_log_write();


    int m_split_lines;  //日志最大行数
    bool m_is_async;   //是否异步，多线程记录日志
    locker m_mutex;     //是否加锁

    int m_log_buf_size;   //日志缓冲区
    char* m_buf;            
    long long m_lines_count; //日志的某一行

    FILE *m_fp;
    char dir_name[128];
    char log_name[128];
    int m_today;                                        
    
    Blockqueue<string>* m_log_queue;



};


#define LOG_DEBUG(format, ...)  {Log::instanse()->log_write(0, format, ##__VA_ARGS__); Log::instanse()->flush();}
#define LOG_INFO(format, ...)  {Log::instanse()->log_write(1, format, ##__VA_ARGS__); Log::instanse()->flush();}
#define LOG_WARN(format, ...)  {Log::instanse()->log_write(2, format, ##__VA_ARGS__); Log::instanse()->flush();}
#define LOG_ERROR(format, ...)  {Log::instanse()->log_write(3, format, ##__VA_ARGS__); Log::instanse()->flush();}