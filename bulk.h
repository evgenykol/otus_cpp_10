#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctime>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <future>
#include <atomic>

using namespace std;

namespace bulk{

class Metrics
{
public:
    size_t blocks;
    size_t commands;

    Metrics()
    {
        blocks = 0;
        commands = 0;
    }
    Metrics(size_t blocks_, size_t commands) : blocks(blocks_), commands(commands) {}

    const Metrics operator+ (const Metrics &rhs)
    {
        return Metrics(this->blocks + rhs.blocks, this->commands + rhs.commands);
    }

    Metrics& operator +=(const Metrics &rhs)
    {
        this->blocks += rhs.blocks;
        this->commands += rhs.commands;
        return *this;
    }
};

class Commands
{
public:
    vector<string> cmds;
    time_t timestamp;
    Metrics metrics;

    void push_back(string str);
    void push_back_block(string str);
    void clear();
};

class Observer
{

public:
    Observer()
    {
        run_flag = true;
    }

    ~Observer()
    {
        run_flag = false;
    }
    Metrics metrics;

    virtual void dump(Commands &cmd) = 0;
    atomic<bool> run_flag;
};

class Dumper
{
    vector<Observer *> subs;
public:
    void subscribe(Observer *ob);
    void notify(Commands &cmd);
    void dump_commands(Commands &cmd);
    void stop_dumping();
};

class ConsoleDumper : public Observer
{
    mutex m;
    condition_variable cv;
    bool flag;
    Commands commands;
public:

    ConsoleDumper(Dumper *dmp);
    void dump(Commands &cmd);
    void dumper();
};

class FileDumper : public Observer
{
public:

    FileDumper(Dumper *dmp);
    void dump(Commands &cmd);
    string get_unique_number();
};


class BulkContext
{
    static constexpr char delimiter = '\n';
    size_t bulk_size;
    Dumper* dumper;

    Commands cmds;
    Metrics metrics;
    size_t lines_count;

    string input_line_tail;
    bool blockFound = false;
    int nestedBlocksCount = 0;

public:
    ConsoleDumper* conDumper;
    FileDumper* fileDumper;

    BulkContext(size_t bulk_size);
    ~BulkContext();

    void process_input(const char *line, size_t size);
    void add_line(string &cmd);
    void end_input();
    void print_metrics();
};

}
