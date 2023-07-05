# Crashlogs
A simple way to output stack traces when a program crashes in C++, using the new C++23 `<stacktrace>` header.  
Works on Windows. Kind of works on Linux.

## Usage

include the files in your project, then call  
Call `glaiel::crashlogs::begin_monitoring` to enable crash handling (probablly do this at the start of your program).  
Monitoring and writing the crashlog is done in a worker thread.

when the program crashes, it will save a timestamped stack trace to the folder specified with `glaiel::crashlogs::set_crashlog_folder(folder_path);`

some additional customizability and callbacks are documented in the header file


note: the stack trace outputted will include a bunch of error handling stuff at the top. It would be nice to skip printing the first X stack trace entries, but how many to skip seems kinda dependent on optimization settings and which condition triggered the error handler, so I did not bother with that yet.

<!-- TODO add example output (preferably from windows) -->

### Linux (GCC) support

*Note that at the time of writing GCC (13.1.1) and its implementation of the C++ standard library consider the backtrace feature to be experimental!*  
This means that you need to manually link against `stdc++_libbacktrace`.

Also the backtraces currently look like this
```
Received signal 11 SIGSEGV
   0#      at :32588
   1#      at :32588
   2#      at :32588
   3#      at :32588
   4#      at :32588
   5#      at :32588
   6#      at :32588
   7# 
```
like they're pretty useless, I'd stick to a different solution until GCC has finished the backtrace feature fully.

### Testing

`TestTool.cpp` is a very simple command line tool to test a few different cases.  
It will enable the monitoring and delibrately crash so you can manually verify the handler works.
