#include "bulk.h"

using namespace std;
using namespace bulk;
using namespace std::chrono_literals;

namespace bulk {
    thread_local Metrics log_metrics;
}

void Commands::push_back(string str)
{
    if(!cmds.size())
    {
        timestamp = time(nullptr);
    }
    cmds.push_back(str);
    ++metrics.commands;
}

void Commands::push_back_block(string str)
{
    if(!cmds.size())
    {
        timestamp = time(nullptr);
    }
    cmds.push_back(str);
    ++metrics.commands;
}

void Commands::clear()
{
    cmds.clear();
    metrics.commands = 0;
    metrics.blocks = 0;
}

Dumper::Dumper()
{
    cout << "ctor Dumper" << endl;
}

Dumper::~Dumper()
{
    cout << "dtor Dumper" << endl;
}

void Dumper::subscribe(Observer *ob)
{
    subs.push_back(ob);
}

void Dumper::notify(Commands &cmds)
{
    for (auto s : subs)
    {
        s->dump(cmds);
    }
}

void Dumper::dump_commands(Commands &cmd)
{
    notify(cmd);
}

void Dumper::stop_dumping()
{
    for (auto s : subs)
    {
        s->stop();
    }
}

ConsoleDumper::ConsoleDumper(shared_ptr<Dumper> dmp)
{
    cout << "ctor ConsoleDumper" << endl;
    dmp->subscribe(this);
}

ConsoleDumper::~ConsoleDumper()
{
    cout << "dtor ConsoleDumper" << endl;
}

void ConsoleDumper::dump(Commands &cmd)
{
    cout << "ConsoleDumper::dump" << endl;
    {
        lock_guard<mutex> lg(m);
        q.push(cmd);
    }
    cv.notify_one();
}

void ConsoleDumper::stop()
{
    cout << "ConsoleDumper::stop" << endl;
    {
        lock_guard<mutex> lg(m);
        run_flag = false;
    }
    cv.notify_all();
}

void ConsoleDumper::dumper(Metrics &metrics)
{
    while (run_flag || !q.empty())
    {
        unique_lock<mutex> lk(m);
        cv.wait(lk, [this]{return (!run_flag || !q.empty());});
//        cout << "con dump " << run_flag << " " << q.empty() << endl;
        if(!run_flag && q.empty())
        {
            return;
        }

        auto commands = q.front();
        q.pop();
        //lk.unlock();

        metrics += commands.metrics;
        bool is_first = true;
        cout << "bulk: ";
        for(auto s : commands.cmds)
        {
            if(is_first)
            {
                is_first = false;
            }
            else
            {
                cout << ", ";
            }
            cout << s;
        }
        cout << endl;
    }
//    while (run_flag)
//    {
//        unique_lock<mutex> lk(m);
//        cv.wait(lk, [this]{return flag;});

//        if(commands.cmds.size())
//        {
//            log_metrics += commands.metrics;
//            bool is_first = true;
//            cout << "bulk: ";
//            for(auto s : commands.cmds)
//            {
//                if(is_first)
//                {
//                    is_first = false;
//                }
//                else
//                {
//                    cout << ", ";
//                }
//                cout << s;
//            }
//            cout << endl;
//        }
//        flag = false;
//    }
//    return log_metrics;
}

FileDumper::FileDumper(shared_ptr<Dumper> dmp)
{
    cout << "ctor FileDumper" << endl;
    dmp->subscribe(this);
}

FileDumper::~FileDumper()
{
    cout << "dtor FileDumper" << endl;
}

string FileDumper::get_unique_number()
{
    static int unique_file_counter = 0;
    return to_string(++unique_file_counter);
}

void FileDumper::dump(Commands &cmd)
{
    cout << "FileDumper::dump" << endl;
    {
        lock_guard<mutex> lg(m);
        q.push(cmd);
    }
    cv.notify_one();
}

void FileDumper::stop()
{
    cout << "FileDumper::stop" << endl;
    {
        lock_guard<mutex> lg(m);
        run_flag = false;
        //flag = true;
        //commands.clear();
    }
    cv.notify_all();
}

void FileDumper::dumper(Metrics &metrics)
{
    while (run_flag || !q.empty())
    {
        unique_lock<mutex> lk(m);
        cv.wait(lk, [this]{return (!run_flag || !q.empty());});
//        cout << "file dump " << run_flag << " " << q.empty() << endl;
        if(!run_flag && q.empty())
        {
            return;
        }

        auto cmds = q.front();
        q.pop();
        //lk.unlock();

        metrics += cmds.metrics;

        string filename = "bulk" + to_string(cmds.timestamp) + "_" + get_unique_number() + ".log";
        ofstream of(filename);

        bool is_first = true;
        of << "bulk: ";
        for(auto s : cmds.cmds)
        {
            if(is_first)
            {
                is_first = false;
            }
            else
            {
                of << ", ";
            }
            of << s;
        }
        of << endl;
        of.close();
    }
}

BulkContext::BulkContext(size_t bulk_size_)
{
    cout << "ctor BulkContext" << endl;
    bulk_size = bulk_size_;
    blockFound = false;
    nestedBlocksCount = 0;
    lines_count = 0;

    dumper = make_shared<Dumper>();
    conDumper = make_shared<ConsoleDumper>(dumper);
    fileDumper = make_shared<FileDumper>(dumper);
}

//BulkContext::BulkContext(size_t bulk_size_, queue<Commands> *cq, queue<Commands> *fq,
//                    mutex *cm, mutex *fm,
//                    condition_variable *ccv, condition_variable *fcv)
//{
//    cout << "ctor BulkContext 1" << endl;
//    bulk_size = bulk_size_;
//    blockFound = false;
//    nestedBlocksCount = 0;
//    lines_count = 0;

//    dumper = make_shared<Dumper>();

//    console_queue = cq;
//    file_queue = fq;

//    console_mutex = cm;
//    file_mutex = fm;

//    console_cv = ccv;
//    file_cv = fcv;
//}

BulkContext::~BulkContext()
{
    cout << "dtor BulkContext" << endl;
}

void BulkContext::add_line(string &cmd)
{
    ++lines_count;
    if((cmd != "{") && !blockFound)
    {
        cmds.push_back(cmd);

        if(cmds.metrics.commands == bulk_size)
        {
            dumper->dump_commands(cmds);
            //dump(cmds);
            metrics += cmds.metrics;
            cmds.clear();
        }
    }
    else
    {
        if(!blockFound)
        {
            blockFound = true;
            if(cmds.metrics.commands)
            {
                dumper->dump_commands(cmds);
                //dump(cmds);
                metrics += cmds.metrics;
                cmds.clear();
            }
            return;
        }

        if(cmd == "{")
        {
            ++nestedBlocksCount;
        }
        else if(cmd == "}")
        {
            if (nestedBlocksCount > 0)
            {
                --nestedBlocksCount;
                ++cmds.metrics.blocks;
            }
            else
            {
                ++cmds.metrics.blocks;
                dumper->dump_commands(cmds);
                //dump(cmds);
                metrics += cmds.metrics;
                cmds.clear();
                blockFound = false;
            }
        }
        else
        {
            cmds.push_back_block(cmd);
        }
    }
}

void BulkContext::end_input()
{
    if(cmds.metrics.commands)
    {
        dumper->dump_commands(cmds);
        //dump(cmds);
        metrics.commands += cmds.metrics.commands;
    }
    dumper->stop_dumping();
}

void BulkContext::dump(Commands cmd)
{
//    {
//        lock_guard<mutex> lgc(*console_mutex);
//        console_queue->push(cmd);
//    }
//    console_cv->notify_one();

//    {
//        lock_guard<mutex> lgf(*file_mutex);
//        file_queue->push(cmd);
//    }
//    file_cv->notify_one();
}

void BulkContext::print_metrics()
{
    cout << "main: " << lines_count << " lines, "
                     << metrics.commands << " commands, "
                     << metrics.blocks << " blocks"
                     << endl;
}


