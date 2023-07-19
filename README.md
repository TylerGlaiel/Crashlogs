# Crashlogs
A simple way to output stack traces when a program crashes in C++, using the new C++23 &lt;stacktrace> header
currently windows-only, but if anyone wants to add mac or linux support I'll merge it in

usage: include the files in your project, then call  
glaiel::crashlogs::begin_monitoring();  
to enable crash handling (probably do this at the start of your program)

when the program crashes, it will save a timestamped stack trace to the folder specified with  
glaiel::crashlogs::set_crashlog_folder(folder_path);

some additional customizability and callbacks are documented in the header file


note: the stack trace outputted will include a bunch of error handling stuff at the top. It would be nice to skip printing the first X stack trace entries, but how many to skip seems kinda dependent on optimization settings and which condition triggered the error handler, so I did not bother with that yet.

### Testing

`TestTool.cpp` is a very simple command line tool to test a few different cases.  
It will enable the monitoring and delibrately crash so you can manually verify the handler works.
