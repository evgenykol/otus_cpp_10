#include "bulk.h"

using namespace std;
using namespace bulk;

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
        s->run_flag = false;
    }
}


ConsoleDumper::ConsoleDumper(Dumper *dmp)
{
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

void ConsoleDumper::dumper()
{
    while (run_flag)
    {
        unique_lock<mutex> lk(m);
        cv.wait(lk, [this]{return flag;});

        if(commands.cmds.size())
        {
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
        flag = false;
    }
}


FileDumper::FileDumper(Dumper *dmp)
{
    dmp->subscribe(this);
}


string FileDumper::get_unique_number()
{
    static int unique_file_counter = 0;
    return to_string(++unique_file_counter);
}

void FileDumper::dump(Commands &cmd)
{
    string filename = "bulk" + to_string(cmd.timestamp) + "_" + get_unique_number() + ".log";
    ofstream of(filename);

    bool is_first = true;
    of << "bulk: ";
    for(auto s : cmd.cmds)
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

BulkContext::BulkContext(size_t bulk_size_)
{
    //cout << "ctor BulkContext" << endl;
    bulk_size = bulk_size_;
    dumper = new Dumper();
    conDumper = new ConsoleDumper(dumper);
    fileDumper = new FileDumper(dumper);
}

BulkContext::~BulkContext()
{
    //cout << "dtor BulkContext" << endl;
    delete dumper;
    delete conDumper;
    delete fileDumper;
}

void BulkContext::process_input(const char *line, size_t size)
{
    //cout << "processLine: " << line << endl;
    string cur_line = input_line_tail;
    input_line_tail.clear();

    for(int i = 0; i < size; ++i)
    {
        if(line[i] != delimiter)
        {
            cur_line.push_back(line[i]);
        }
        else
        {
            add_line(cur_line);
            cur_line.clear();
        }
    }

    //Если что-то осталось (не пришла целая команда) - сохраним
    if(cur_line.size())
    {
        input_line_tail = cur_line;
    }
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
        dumper->stop_dumping();

    }
}

void BulkContext::print_metrics()
{
    cout << "main: " << lines_count << " lines, "
                     << metrics.commands << " commands, "
                     << metrics.blocks << " blocks"
                     << endl;

    cout << "log: " << conDumper->metrics.commands << " commands, "
                    << conDumper->metrics.blocks << " blocks"
                    << endl;
}


