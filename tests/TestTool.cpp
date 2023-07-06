// A very simple CLI tool to manually test the different cases of Crashlog.
// It's not really possible to unit test crashing, so this will have to suffice.

#include <iostream>
#include <thread>
#include <optional>
#include <string_view>

#include <csignal>

#include "../crashlogs.h"

enum class TestType
{
    Segfault,
    Abort,
    Terminate,
    IllegalInstruction,
    UnhandledException,
    StackOverflow,
};

// forward declarations so we can use these in main
void usage(const char *arg0);
[[nodiscard]] std::optional<TestType> testTypeFromString(std::string_view str);
void causeStackOverflow(int val);

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        usage(argv[0]);
        return 1;
    }
    const auto testTypeParsed = testTypeFromString(argv[1]);
    if (!testTypeParsed.has_value())
    {
        std::cerr << "Cannot parse `" << argv[1] << "` as a valid test type!\n"
                  << std::endl;
        usage(argv[0]);
        return 2;
    }
    const TestType testType = testTypeParsed.value();

    std::cout << "Initializing crashlogs" << std::endl;

    glaiel::crashlogs::set_crashlog_folder("./test_logs");
    glaiel::crashlogs::begin_monitoring();

    std::cout << "Gonna wait a second..." << std::endl;

    for (int cnt = 0; cnt < 10; cnt++)
    {
        std::cout << ".";
        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << std::endl;

    switch (testType)
    {
    case TestType::Segfault:
    {
        std::cout << "Causing Segfault" << std::endl;
        int *a = nullptr;
        int b = *a + 1;
    }
    break;
    case TestType::Abort:
    {
        std::cout << "Causing Abort" << std::endl;
        std::abort();
    }
    break;
    case TestType::Terminate:
    {
        std::cout << "Causing Terminate" << std::endl;
        std::terminate();
    }
    break;
    case TestType::IllegalInstruction:
    {
        std::cout << "Causing IllegalInstruction" << std::endl;
        // no idea how to cause this delibrately so I'm just raising the signal manually
        std::raise(SIGILL);
    }
    break;
    case TestType::UnhandledException:
    {
        std::cout << "Causing UnhandledException" << std::endl;
        throw std::logic_error("Whoops");
    }
    break;
    case TestType::StackOverflow:
    {
        std::cout << "Causing StackOverflow" << std::endl;
        causeStackOverflow(0);
    }
    break;
    }

    return 0;
}

void usage(const char *arg0)
{
    std::cerr << "Usage: \n";
    std::cerr << "\t" << arg0 << " <TestType>\n";
    std::cerr << "\n"
                 "\tWith valid test types:\n"
                 "\t - 'Segfault'\n"
                 "\t - 'Abort'\n"
                 "\t - 'Terminate'\n"
                 "\t - 'IllegalInstruction'\n"
                 "\t - 'UnhandledException'\n"
                 "\t - 'StackOverflow'"
              << std::endl;
}

std::optional<TestType> testTypeFromString(std::string_view str)
{
    if (str == "Segfault" || str == "segfault")
        return TestType::Segfault;
    if (str == "Abort" || str == "abort")
        return TestType::Abort;
    if (str == "Terminate" || str == "terminate")
        return TestType::Terminate;
    if (str == "IllegalInstruction" || str == "illegalinstruction")
        return TestType::IllegalInstruction;
    if (str == "UnhandledException" || str == "unhandledexception")
        return TestType::UnhandledException;
    if (str == "StackOverflow" || str == "stackoverflow")
        return TestType::StackOverflow;
    return std::nullopt;
}

void causeStackOverflow(int val)
{
    causeStackOverflow(val + 1);
    std::cout << " " << val;
}
