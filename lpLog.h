//
//  lpLog.h
//  lpLog
//
//  Created by zhTian on 2017/8/14.
//  Copyright © 2017年 zhTian. All rights reserved.
//

#ifndef lpLog_h
#define lpLog_h

#include <list> //double-linked list
#include <string>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <sys/stat.h>
#include <future>         // std::async, std::future
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

#define MAXBUFFER 1024

enum LEVEL {
    warn,
    error,
};

namespace LOG {
    
    //实际存放log数据的buffer
    struct buffer {
        buffer() {
            _data = new char[MAXBUFFER];
            _used_len = 0;
            _total_len = MAXBUFFER;
        }
        void append(const char *s, size_t len) {
            if (avai_len() < len)
                return ;
            memcpy(_data + _used_len, s, len);
            
            _used_len += len;

        }
        void do_persist(FILE *fp) {
            fwrite(_data, _used_len, 1, fp);
        }
        
        size_t avai_len() {
            return _total_len - _used_len;
        }
        
        char *_data;
        size_t _used_len;
        size_t _total_len;
    };
    
    
    class lpLog {
    public:
        static lpLog *inst() {
            static lpLog *_instance = new lpLog();
            return _instance;
        }
        
    private:
        lpLog() {
            _cur_buf = new buffer();
            _level = error;
            _f = false;
        }
        
        lpLog(const lpLog&) = delete;
        void operator=(const lpLog&) = delete;
        ~lpLog(){}
        
    public:
        void init_path(const char *dir, const char *name, LEVEL level);
        void persist();
        void append_log(const char *fmt, ...);
        LEVEL get_level() {
            return _level;
        }
        
    public:
        void flush() {
            _f = true;
            _cond.notify_all();
        }
        
        void getNowTime()
        {
            timespec time;
            clock_gettime(CLOCK_REALTIME, &time);  //获取相对于1970到现在的秒数
            tm nowTime;
            localtime_r(&time.tv_sec, &nowTime);
            sprintf(current, "%04d%02d%02d%02d%02d%02d", nowTime.tm_year + 1900, nowTime.tm_mon+1, nowTime.tm_mday,
                    nowTime.tm_hour, nowTime.tm_min, nowTime.tm_sec);
        }
        
    private:
        std::list<buffer*> _log_list;
        
        std::mutex _mutex;
        std::condition_variable _cond;
        
        char _dir[255];
        char _file_name[255];
        LEVEL _level;
        
        buffer *_cur_buf;
        bool _f;
        FILE *_fp;
        
        char current[MAXBUFFER];
    };
}

//global functionss
void *do_persist() {
    LOG::lpLog::inst()->persist();
    return nullptr;
}

pid_t pid() {
    return getpid();
}


void
LOG::lpLog::persist() {
    while(true) {
        std::unique_lock<std::mutex> lk(_mutex);
        while(!_f) {
            _cond.wait(lk);
        }
        
        for(buffer *buf : _log_list) {
            buf->do_persist(_fp);
        }
        
        _log_list.clear();
    }
}

void
LOG::lpLog::append_log(const char *fmt, ...) {    
    va_list va;
    va_start(va, fmt);
    
    char logs[MAXBUFFER];
    size_t len = vsnprintf(logs, MAXBUFFER, fmt, va);
    
    va_end(va);
    
    std::lock_guard<std::mutex> lk(_mutex);
    
    if(_cur_buf->avai_len() > len) {
        _cur_buf->append(logs, len);
    }
    else {
        _log_list.push_back(_cur_buf);
        
        _f = true;
        _cond.notify_all();
    }
}

void
LOG::lpLog::init_path(const char *dir, const char *name, LEVEL level) {

    std::lock_guard<std::mutex> lk_guard(_mutex);
    
    strcpy(_dir, dir);
    strcpy(_file_name, name);
    _level = level;
    
    mkdir(dir, 0777);
    
    char log_path[1024] = {};
    getNowTime();
    sprintf(log_path, "%s/%s.%s.log", _dir, _file_name, current);
    _fp = fopen(log_path, "w");
}


//宏定义
#define INIT_LOG(dir, name, level) \
do \
{  \
    LOG::lpLog::inst()->init_path(dir, name, level); \
    std::thread t(do_persist); \
    t.detach(); \
}while(0)

#define ERR(fmt, args...) \
do \
{  \
    LOG::lpLog::inst()->append_log("[%u]%s:%d(%s): " fmt "\n" , pid(), __FILE__, __LINE__, __FUNCTION__, ##args); \
}while(0)

#endif /* lpLog_h */











