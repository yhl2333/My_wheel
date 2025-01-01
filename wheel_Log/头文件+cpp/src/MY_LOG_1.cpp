#include "MY_LOG_1.h"




// 默认构造函数
sem::sem() {
    if (sem_init(&m_sem, 0, 0) != 0) {
        throw std::exception();
    }
}

// 带参数的构造函数
sem::sem(int num) {
    if (sem_init(&m_sem, 0, num) != 0) {
        throw std::exception();
    }
}

// 析构函数，销毁信号量
sem::~sem() {
    sem_destroy(&m_sem);
}

// 等待信号量
bool sem::wait() {
    return sem_wait(&m_sem) == 0;
}

// 释放信号量
bool sem::post() {
    return sem_post(&m_sem) == 0;
}








// 构造函数，初始化互斥锁
locker::locker() {
    if (pthread_mutex_init(&m_mutex, NULL) != 0) {
        throw std::exception();
    }
}

// 析构函数，销毁互斥锁
locker::~locker() {
    pthread_mutex_destroy(&m_mutex);
}

// 加锁操作
bool locker::lock() {
    return pthread_mutex_lock(&m_mutex) == 0;
}

// 解锁操作
bool locker::unlock() {
    return pthread_mutex_unlock(&m_mutex) == 0;
}

// 获取互斥锁对象的指针
pthread_mutex_t* locker::get() {
    return &m_mutex;
}




// 构造函数，初始化条件变量
cond::cond() {
    if (pthread_cond_init(&m_cond, NULL) != 0) {
        throw std::exception();
    }
}

// 析构函数，销毁条件变量
cond::~cond() {
    pthread_cond_destroy(&m_cond);
}

// 等待条件变量
bool cond::wait(pthread_mutex_t* m_mutex) {
    return pthread_cond_wait(&m_cond, m_mutex) == 0;
}

// 带超时的等待条件变量
bool cond::timewait(pthread_mutex_t* m_mutex, struct timespec t) {
    return pthread_cond_timedwait(&m_cond, m_mutex, &t) == 0;
}

// 发送信号，唤醒一个等待的线程
bool cond::signal() {
    return pthread_cond_signal(&m_cond) == 0;
}

// 广播信号，唤醒所有等待的线程
bool cond::broadcast() {
    return pthread_cond_broadcast(&m_cond) == 0;
}




// 构造函数，初始化队列大小
template <class T>
Blockqueue<T>::Blockqueue(int queue_max_size) : m_max_size(queue_max_size) {
    m_array = new T[m_max_size];
    m_front = -1;
    m_back = -1;
    m_size = 0;
}

// 析构函数，销毁队列
template <class T>
Blockqueue<T>::~Blockqueue() {
    delete[] m_array;
    m_array = nullptr;
}

// 插入元素到队列
template <class T>
bool Blockqueue<T>::push(const T& item) {
    m_locker.lock();       
    while(m_size>=m_max_size)
    {
        m_cond_Prod.wait(m_locker.get());
    }


    m_front = (m_front+1)%m_max_size;
    m_array[m_front%m_max_size] = item;
    m_size++;
    m_cond_Cons.signal();
    m_locker.unlock();
    std::cout<<m_front<<std::endl;
    return 1;
}

// 从队列中弹出元素（带返回值）
template <class T>
bool Blockqueue<T>::pop(T& item) {
    m_locker.lock();

    while(m_size <= 0)
    {   
        m_cond_Cons.wait(m_locker.get());
    }
            
    m_back = (m_back+1)%m_max_size;
    item = m_array[m_back];
    m_size--;
    m_cond_Prod.signal();
    m_locker.unlock();
    return 1;
}

// 从队列中弹出元素（不带返回值）
template <class T>
bool Blockqueue<T>::pop() {
    m_locker.lock();

    while(m_size <= 0)
    {   
        m_cond_Cons.wait(m_locker.get());
    }

    m_back = (m_back+1)%m_max_size;
    m_size--;
    m_cond_Prod.signal();
    m_locker.unlock();
    return 1;
}

// 获取队列的前端元素
template <class T>
bool Blockqueue<T>::front(T& item) {
    m_locker.lock();
    if (m_size != 0) {
        item = m_array[m_front];
        m_locker.unlock();
        return true;
    }
    m_locker.unlock();
    return false;
}

// 获取队列的后端元素
template <class T>
bool Blockqueue<T>::back(T& item) {
    m_locker.lock();
    if (m_size != 0) {
        item = m_array[m_back];
        m_locker.unlock();
        return true;
    }
    m_locker.unlock();
    return false;
}

// 判断队列是否已满
template <class T>
bool Blockqueue<T>::full() {
    m_locker.lock();
    bool is_full = (m_size >= m_max_size);
    m_locker.unlock();
    return is_full;
}

// 判断队列是否为空
template <class T>
bool Blockqueue<T>::empty() {
    m_locker.lock();
    bool is_empty = (m_size == 0);
    m_locker.unlock();
    return is_empty;
}

// 获取队列当前大小
template <class T>
int Blockqueue<T>::size() {
    return m_size;  // 非线程安全方法
}













// 获取单例对象
Log* Log::instanse() {
    static Log* log_Base = new Log();
    return log_Base;
}

// 异步写日志的公有接口，调用该接口创建写日志的线程
void *Log::write_log_thread(void *args) {
    Log::instanse()->asyn_log_write();
    return nullptr;
}

// 构造函数，初始化一些参数
Log::Log() : dir_name{0}, log_name{0}, m_lines_count(0), m_is_async(false) {}

// 析构函数，释放资源
Log::~Log() {
    if (m_fp) {
        fclose(m_fp);
    }
    if (m_buf) {
        delete[] m_buf;
    }
}

// 初始化日志类，包括文件名，日志缓冲区大小，最大行数，最大阻塞队列长度
bool Log::init(const char* filename, int log_buf_size, int split_lines, int max_queue_size) {
    // 如果设置了最大队列长度，则设置为异步写日志
    if (max_queue_size >= 1) {
        m_is_async = true;
        m_log_queue = new Blockqueue<string>(max_queue_size);
        pthread_t tid;
        pthread_create(&tid, NULL, write_log_thread, NULL);
        pthread_detach(tid);
    }

    m_log_buf_size = log_buf_size;
    m_buf = new char[m_log_buf_size];
    memset(m_buf, '\0', m_log_buf_size);
    m_split_lines = split_lines;

    time_t t = time(NULL);
    struct tm* sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    const char* p = strrchr(filename, '/');
    char new_log_file_name[256] = {0};

    // 如果输入的文件名没有 / ，说明是当前目录下的文件
    if (p == NULL) {
        snprintf(new_log_file_name, 255, "%d_%02d_%02d_%s%s", 
                 my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, filename, ".0");
    } else {
        strcpy(log_name, p + 1);
        strncpy(dir_name, filename, p - filename + 1);   
        snprintf(new_log_file_name, 255, "%s%d_%02d_%02d_%s%s", 
                 dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name, ".0");
    }

    m_today = my_tm.tm_mday;
    m_fp = fopen(new_log_file_name, "a");

    if (m_fp == NULL) {
        return false;
    }

    return true;
}

// 写日志
void Log::log_write(int level, const char* format, ...) {

    // struct timeval {
    //     time_t      tv_sec;   // 秒数
    //     suseconds_t tv_usec;  // 微秒数
    // };
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;

        // struct tm {
        //     int tm_sec;    // 秒，范围从 0 到 60
        //     int tm_min;    // 分，范围从 0 到 59
        //     int tm_hour;   // 小时，范围从 0 到 23
        //     int tm_mday;   // 一个月中的某天，范围从 1 到 31
        //     int tm_mon;    // 月份，范围从 0 到 11 (0 表示一月)
        //     int tm_year;   // 自 1900 年起的年数
        //     int tm_wday;   // 一周中的某天，范围从 0 到 6 (0 表示星期天)
        //     int tm_yday;   // 一年中的某天，范围从 0 到 365
        //     int tm_isdst;  // 夏令时标志
        // };



    struct tm* sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    char s[16] = {0};
    switch (level) {
        case 0: strcpy(s, "[debug]:"); break;
        case 1: strcpy(s, "[info]:"); break;
        case 2: strcpy(s, "[warn]:"); break;
        case 3: strcpy(s, "[erro]:"); break;
        default: strcpy(s, "[info]:"); break;
    }

    m_mutex.lock();

    // 每次写入前判断当前日志日期是否为当天，是否超出最大行数
    if (m_today != my_tm.tm_mday || (m_lines_count % m_split_lines == 0 && m_lines_count != 0)) {
        char new_log[256] = {0};
        fflush(m_fp);
        fclose(m_fp);
        char tail[16] = {0};
        snprintf(tail, 16, "%d-%02d-%02d-", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);

        if (m_today != my_tm.tm_mday) {
            snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name);
            m_today = my_tm.tm_mday;
            m_lines_count = 0;
        } else {
            snprintf(new_log, 255, "%s%s%s.%lld", dir_name, tail, log_name, m_lines_count / m_split_lines);
        }

        m_fp = fopen(new_log, "a");
    }

    m_lines_count++;
    m_mutex.unlock();

    va_list valist;
    va_start(valist, format);

    string log_str;
    m_mutex.lock();

    // 写入具体的时间内容格式
    int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);

    int m = vsnprintf(m_buf + n, m_log_buf_size - n - 1, format, valist);//这个-1留给/0；
    m_buf[n + m] = '\n';    //这里把原来的/0变为/n
    m_buf[n + m + 1] = '\0';//后面再加一个/0
    log_str = m_buf;

    m_mutex.unlock();

    // 如果是异步模式且阻塞队列不满，将日志内容加入队列
    if (m_is_async) {
        m_log_queue->push(log_str);
    } else {
        m_mutex.lock();
        fputs(log_str.c_str(), m_fp);
        m_mutex.unlock();
    }

    va_end(valist);
}

// 强制刷新缓冲区
void Log::flush() {
    m_mutex.lock();
    fflush(m_fp);
    m_mutex.unlock();
}

// 异步写日志
void Log::asyn_log_write() {
    std::cout << "Current thread ID: " << pthread_self() << std::endl;
    string single_log;
    while (m_log_queue->pop(single_log)) {
        m_mutex.lock();
        fputs(single_log.c_str(), m_fp);
        m_mutex.unlock();
    }
}
