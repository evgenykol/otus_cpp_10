#include <iostream>
#include "string.h"

#include "version.h"
#include "bulk.h"

using namespace std;

int main(int argc, char *argv[])
{
    try
    {
        int commandsCount = 0;
        if ((argc > 1) &&
           (!strncmp(argv[1], "-v", 2) || !strncmp(argv[1], "--version", 9)))
        {
            cout << "version " << version() << endl;
            return 0;
        }
        else if (argc > 1) {
            commandsCount = atoi(argv[1]);
        }
        else
        {
            cout << "bad input" << endl;
            return 0;
        }

        freopen("cmd_input.txt", "rt", stdin);

        auto blk = bulk::BulkContext(commandsCount);

        auto cdt  = async(std::launch::async, &bulk::ConsoleDumper::dumper, blk.conDumper);
        auto fdt1 = async(std::launch::async, &bulk::FileDumper::dumper, blk.fileDumper);
        auto fdt2 = async(std::launch::async, &bulk::FileDumper::dumper, blk.fileDumper);

        string line;
        while(getline(cin, line))
        {
            blk.add_line(line);
        }
        blk.end_input();

        cout << endl;
        blk.print_metrics();
        cout << "log ->" << endl;
        bulk::Metrics::print_metrics(cdt.get(), "log");
        cout << "file1 ->" << endl;
        bulk::Metrics::print_metrics(cdt.get(), "file1");
        cout << "file2 ->" << endl;
        bulk::Metrics::print_metrics(cdt.get(), "file2");

    }
    catch(exception &e)
    {
        cout << "exception: " << e.what() << endl;
    }
    return 0;
}
