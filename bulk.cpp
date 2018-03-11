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

void ConsoleDumper::dump(Commands &cmd)
{
    {
        lock_guard<mutex> lg(m);
        commands = cmd;
        flag = true;
    }
    cv.notify_one();
}

void ConsoleDumper::stop()
{
    {
        lock_guard<mutex> lg(m);
        run_flag = false;
        flag = true;
        commands.clear();
    }
    cv.notify_all();
}

Metrics ConsoleDumper::dumper()
{
    while (run_flag)
    {
        unique_lock<mutex> lk(m);
        cv.wait(lk, [this]{return flag;});

        if(commands.cmds.size())
        {
            log_metrics += commands.metrics;
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
        flag = false;
    }
    return log_metrics;
}


FileDumper::FileDumper(shared_ptr<Dumper> dmp)
{
    cout << "ctor FileDumper" << endl;
    dmp->subscribe(this);
}


string FileDumper::get_unique_number()
{
    static int unique_file_counter = 0;
    return to_string(++unique_file_counter);
}

void FileDumper::dump(Commands &cmd)
{
    {
        lock_guard<mutex> lg(m);
        commands.push(cmd);
    }
    cv.notify_one();
}

void FileDumper::stop()
{
    {
        lock_guard<mutex> lg(m);
        run_flag = false;
        //flag = true;
        //commands.clear();
    }
    cv.notify_all();
}

Metrics FileDumper::dumper()
{
    while (run_flag)
    {
        unique_lock<mutex> lk(m);
        cv.wait(lk, [this]{return commands.size();});

        auto cmds = commands.front();
        commands.pop();
        //lk.unlock();

        log_metrics += cmds.metrics;

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
    return log_metrics;
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
        metrics.commands += cmds.metrics.commands;
    }
    dumper->stop_dumping();
}

void BulkContext::print_metrics()
{
    cout << "main: " << lines_count << " lines, "
                     << metrics.commands << " commands, "
                     << metrics.blocks << " blocks"
                     << endl;
}


