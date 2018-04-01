#include <iostream>
#include <thread>

#include "string.h"

#include "version.h"
#include "bulk.h"

int main(int argc, char *argv[])
{
    try
    {
        int commandsCount = 0;
        if ((argc > 1) &&
           (!strncmp(argv[1], "-v", 2) || !strncmp(argv[1], "--version", 9)))
        {
            std::cout << "version " << version() << std::endl;
            return 0;
        }
        else if (argc > 1)
        {
            commandsCount = std::atoi(argv[1]);
        }
        else
        {
            std::cout << "bad input" << std::endl;
            return 0;
        }

        freopen("cmd_input.txt", "rt", stdin);

        auto blk = bulk::BulkContext(commandsCount);

        bulk::Metrics log_metr, file1_metr, file2_metr;
        std::thread cdt (&bulk::ConsoleDumper::dumper, blk.conDumper, std::ref(log_metr)),
               fdt1 (&bulk::FileDumper::dumper, blk.fileDumper, std::ref(file1_metr)),
               fdt2 (&bulk::FileDumper::dumper, blk.fileDumper, std::ref(file2_metr));

        std::string line;
        while(std::getline(std::cin, line))
        {
            blk.add_line(line);
        }
        blk.end_input();

        cdt.join();
        fdt1.join();
        fdt2.join();

        std::cout << std::endl;
        blk.print_metrics();
        bulk::Metrics::print_metrics(log_metr, "log");
        bulk::Metrics::print_metrics(file1_metr, "file1");
        bulk::Metrics::print_metrics(file2_metr, "file2");
    }
    catch(std::exception &e)
    {
        std::cout << "exception: " << e.what() << std::endl;
    }
    return 0;
}
