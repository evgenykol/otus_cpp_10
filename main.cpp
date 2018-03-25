#include <iostream>
#include "string.h"

#include "version.h"
#include "bulk.h"

using namespace std;
using namespace bulk;

//queue<Commands> console_queue;
//queue<Commands> file_queue;

//mutex console_mutex;
//mutex file_mutex;

//condition_variable console_cv;
//condition_variable file_cv;

//atomic<bool> run_flag;

//void consoleDumper(queue<Commands> &q, Metrics &metrics)
//{
//    while (run_flag || !q.empty())
//    {
//        unique_lock<mutex> lk(console_mutex);
//        console_cv.wait(lk, [&q]{return (!run_flag || !q.empty());});
////        cout << "con dump " << run_flag << " " << q.empty() << endl;
//        if(!run_flag && q.empty())
//        {
//            return;
//        }

//        auto commands = q.front();
//        q.pop();
//        //lk.unlock();

//        metrics += commands.metrics;
//        bool is_first = true;
//        cout << "bulk: ";
//        for(auto s : commands.cmds)
//        {
//            if(is_first)
//            {
//                is_first = false;
//            }
//            else
//            {
//                cout << ", ";
//            }
//            cout << s;
//        }
//        cout << endl;
//    }
//}

//string get_unique_number()
//{
//    static int unique_file_counter = 0;
//    return to_string(++unique_file_counter);
//}

//void fileDumper(queue<Commands> &q, Metrics &metrics)
//{
//    while (run_flag || !q.empty())
//    {
//        unique_lock<mutex> lk(file_mutex);
//        file_cv.wait(lk, [&q]{return (!run_flag || !q.empty());});
////        cout << "file dump " << run_flag << " " << q.empty() << endl;
//        if(!run_flag && q.empty())
//        {
//            return;
//        }

//        auto cmds = q.front();
//        q.pop();
//        //lk.unlock();

//        metrics += cmds.metrics;

//        string filename = "bulk" + to_string(cmds.timestamp) + "_" + get_unique_number() + ".log";
//        ofstream of(filename);

//        bool is_first = true;
//        of << "bulk: ";
//        for(auto s : cmds.cmds)
//        {
//            if(is_first)
//            {
//                is_first = false;
//            }
//            else
//            {
//                of << ", ";
//            }
//            of << s;
//        }
//        of << endl;
//        of.close();
//    }
//}

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

        freopen("cmd_input.txt", "rt", stdin);

        auto blk = bulk::BulkContext(commandsCount);

        //run_flag = true;

        Metrics log_metr, file1_metr, file2_metr;
//        auto cdt  = async(std::launch::async, consoleDumper, ref(console_queue), ref(log_metr));
//        auto fdt1 = async(std::launch::async, fileDumper, ref(file_queue), ref(file1_metr));
//        auto fdt2 = async(std::launch::async, fileDumper, ref(file_queue), ref(file2_metr));

        thread cdt (&ConsoleDumper::dumper, blk.conDumper, ref(log_metr)),
               fdt1 (&FileDumper::dumper, blk.fileDumper, ref(file1_metr)),
               fdt2 (&FileDumper::dumper, blk.fileDumper, ref(file2_metr));

        string line;
        while(getline(cin, line))
        {
            blk.add_line(line);
        }
        blk.end_input();

        //std::this_thread::sleep_for(5s);

//        run_flag = false;
//        console_cv.notify_all();
//        file_cv.notify_all();

        cdt.join();
        fdt1.join();
        fdt2.join();

        cout << endl;
        blk.print_metrics();
        //cout << "log ->" << endl;
        bulk::Metrics::print_metrics(log_metr, "log");
        //cout << "file1 ->" << endl;
        bulk::Metrics::print_metrics(file1_metr, "file1");
        //cout << "file2 ->" << endl;
        bulk::Metrics::print_metrics(file2_metr, "file2");

    }
    catch(exception &e)
    {
        cout << "exception: " << e.what() << endl;
    }
    return 0;
}
