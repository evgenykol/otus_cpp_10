#include <iostream>
#include "string.h"

#include "version.h"
#include "bulk.h"

using namespace std;
using namespace bulk;

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
        else if (argc > 1)
        {
            commandsCount = atoi(argv[1]);
        }
        else
        {
            cout << "bad input" << endl;
            return 0;
        }

        //freopen("cmd_input.txt", "rt", stdin);

        auto blk = bulk::BulkContext(commandsCount);

        Metrics log_metr, file1_metr, file2_metr;
        thread cdt (&ConsoleDumper::dumper, blk.conDumper, ref(log_metr)),
               fdt1 (&FileDumper::dumper, blk.fileDumper, ref(file1_metr)),
               fdt2 (&FileDumper::dumper, blk.fileDumper, ref(file2_metr));

        string line;
        while(getline(cin, line))
        {
            blk.add_line(line);
        }
        blk.end_input();

        cdt.join();
        fdt1.join();
        fdt2.join();

        cout << endl;
        blk.print_metrics();
        Metrics::print_metrics(log_metr, "log");
        Metrics::print_metrics(file1_metr, "file1");
        Metrics::print_metrics(file2_metr, "file2");
    }
    catch(exception &e)
    {
        cout << "exception: " << e.what() << endl;
    }
    return 0;
}
