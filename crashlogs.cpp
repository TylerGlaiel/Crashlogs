#include "crashlogs.h"

//needed to get a stack trace
#include <stacktrace> 

//needed for threading (threading needed to be able to output a call stack during a stack overflow)
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

//needed for being able to output a timestampped crash log
#include <filesystem>
#include <fstream>
#include <ctime>

//needed for being able to get into the crash handler on a crash
#include <csignal>
#include <exception>
#include <cstdlib>
#include <windows.h>

//a decent amount of this was copied/modified from backward.cpp (https://github.com/bombela/backward-cpp)
//mostly the stuff related to actually getting crash handlers on crashes
//and the thread which is SOLELY there to be able to write a log on a stack overflow, 
//since otherwise there is not enough stack space to output the stack trace
//main difference here is utilizing C++23 <stacktrace> header for generating stack traces
//and using <atmoic> and a few other more recent C++ features if we're gonna be using C++23 anyway

namespace glaiel::crashlogs {
    //information for where to save stack traces
    static std::stacktrace trace;
    static std::string header_message;
    static std::filesystem::path output_folder;
    static std::string filename = "crash_{timestamp}.txt";
    static void (*on_output_crashlog)(std::string crashlog_filename) = NULL;

    //thread stuff
    static std::mutex mut;
    static std::condition_variable cv;
    static std::thread output_thread;
    enum class program_status {
        running = 0,
        crashed = 1,
        ending = 2,
        normal_exit = 3,
    };
    static std::atomic<program_status> status = program_status::running;

    //public interface (see header for documentation)
    void set_crashlog_folder(std::string folderpath) {
        output_folder = folderpath;
    }
    void set_crashlog_filename(std::string filename_format) {
        filename = filename_format;
    }
    void set_on_write_crashlog_callback(void(*on_write_crashlog)(std::string)) {
        on_output_crashlog = on_write_crashlog;
    }
    void set_crashlog_header_message(std::string message) {
        std::unique_lock<std::mutex> lk(mut);
        if(status != program_status::running) return;
        header_message = message;
    }
    std::string get_crashlog_header_message() {
        return header_message;
    }

    static std::filesystem::path get_log_filepath();

    //output the crashlog file after a crash has occured
    static void output_crash_log() {
        std::filesystem::path path = get_log_filepath();
        std::ofstream log(get_log_filepath());
        if(!header_message.empty()) log << header_message << std::endl;
        log << trace;
        log.close();

        if(on_output_crashlog) on_output_crashlog(path.string());
    }

    //get the current timestamp as a string, for the crash log filename
    static std::string current_timestamp() {
        std::time_t rawtime;
        std::tm* timeinfo;
        char buffer[80];

        std::time(&rawtime);
        timeinfo = std::localtime(&rawtime);

        std::strftime(buffer, 80, "%Y-%m-%d-%H-%M-%S", timeinfo);
        return buffer;
    }

    //utility function needed for crash log timestamps (replace {timestamp} in filename format with the timestamp string)
    static std::string replace_substr(std::string str, const std::string& search, const std::string& replace) {
        if(search.empty()) return str;
        size_t pos = 0;
        while((pos = str.find(search, pos)) != std::string::npos) {
            str.replace(pos, search.length(), replace);
            pos += replace.length();
        }
        return str;
    }

    //get the crash log filename
    std::filesystem::path get_log_filepath() {
        std::string timestampstr = current_timestamp();
        std::string timestampped_filename = replace_substr(filename, "{timestamp}", timestampstr);

        std::filesystem::path filepath = output_folder / timestampped_filename;

        //ensure the crash log folder exists. error code is here to supporess errors... since we're in an error handler already
        //at default settings this errors because an empty path is specified... lol. but we dont want to create a folder in that case anyway
        std::error_code err;
        std::filesystem::create_directories(output_folder, err);

        return filepath;
    }

    //using a thread here is a hack to get stack space in the case where the crash is a stack overflow
    //this hack was borrowed from backward.cpp
    static void crash_handler_thread() {
        //wait for the program to crash or exit normally
        std::unique_lock<std::mutex> lk(mut);
        cv.wait(lk, [] { return status != program_status::running; });
        lk.unlock();

        //if it crashed, output the crash log
        if(status == program_status::crashed) {
            output_crash_log();
        }

        //alert the crashing thread we're done with the crash log so it can finish crashing
        status = program_status::ending;
        cv.notify_one();
    }

    static inline void crash_handler() {
        //if we crashed during a crash... ignore lol
        if(status != program_status::running) return;

        //save the stacktrace
        trace = std::stacktrace::current();

        //resume the monitoring thread
        status = program_status::crashed; 
        cv.notify_one();

        //wait for the crash log to finish writing
        std::unique_lock<std::mutex> lk(mut);
        cv.wait(lk, [] { return status != program_status::crashed; });
    }

    //various callbacks needed to get into the crash handler during a crash (borrowed from backward.cpp)
    static inline void signal_handler(int) {
        crash_handler();
        abort();
    }
    static inline void terminator() {
        crash_handler();
        abort();
    }
    __declspec(noinline) static LONG WINAPI exception_handler(EXCEPTION_POINTERS*) {
        crash_handler();
        return EXCEPTION_CONTINUE_SEARCH;
    }
    static void __cdecl invalid_parameter_handler(const wchar_t*,const wchar_t*,const wchar_t*,unsigned int,uintptr_t) {
        crash_handler();
        abort();
    }

    //callback needed during a normal exit to shut down the thread
    static inline void normal_exit() {
        status = program_status::normal_exit;
        cv.notify_one();
        output_thread.join();
    }

    //set up all the callbacks needed to get into the crash handler during a crash (borrowed from backward.cpp)
    void begin_monitoring() {
        output_thread = std::thread(crash_handler_thread);

        SetUnhandledExceptionFilter(exception_handler);
        std::signal(SIGABRT, signal_handler);
        std::set_terminate(terminator);
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
        _set_purecall_handler(terminator);
        _set_invalid_parameter_handler(&invalid_parameter_handler);

        std::atexit(normal_exit);
    }
}
