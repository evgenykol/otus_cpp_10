#include <iostream>

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

        auto cdt = async(std::launch::async, &bulk::ConsoleDumper::dumper, blk.conDumper);


        string line;
        while(getline(cin, line))
        {
            blk.add_line(line);
        }
        blk.end_input();
        blk.print_metrics();
    }
    catch(exception &e)
    {
        cout << e.what();
    }
    return 0;
}
