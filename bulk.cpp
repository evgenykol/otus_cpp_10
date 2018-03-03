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
    ++cmdCounter;
}

void Commands::push_back_block(string str)
{
    if(!cmds.size())
    {
        timestamp = time(nullptr);
    }
    cmds.push_back(str);
}

void Commands::clear()
{
    cmds.clear();
    cmdCounter = 0;
}

void Dumper::subscribe(Observer *ob)
{
    subs.push_back(ob);
}

void Dumper::notify()
{
    for (auto s : subs)
    {
        s->dump(cmds);
    }
}

void Dumper::dump_commands(Commands &cmd)
{
    cmds = cmd;
    notify();
}

ConsoleDumper::ConsoleDumper(Dumper *dmp)
{
    dmp->subscribe(this);
}

void ConsoleDumper::dump(Commands &cmd)
{
    bool is_first = true;
    cout << "bulk: ";
    for(auto s : cmd.cmds)
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

BulkContext::BulkContext(size_t bulk_size)
{
    //cout << "ctor BulkContext" << endl;
    commandsCount = bulk_size;
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
            add_command(cur_line);
            cur_line.clear();
        }
    }

    //Если что-то осталось (не пришла целая команда) - сохраним
    if(cur_line.size())
    {
        input_line_tail = cur_line;
    }
}
void BulkContext::add_command(string &cmd)
{
    if((cmd != "{") && !blockFound)
    {
        cmds.push_back(cmd);

        if(cmds.cmdCounter == commandsCount)
        {
            dumper->dump_commands(cmds);
            cmds.clear();
        }
    }
    else
    {
        if(!blockFound)
        {
            blockFound = true;
            if(cmds.cmdCounter)
            {
                dumper->dump_commands(cmds);
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
            }
            else
            {
                dumper->dump_commands(cmds);
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
    if(cmds.cmdCounter)
    {
        dumper->dump_commands(cmds);
    }
}


