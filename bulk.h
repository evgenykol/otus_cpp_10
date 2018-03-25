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
#include <memory>
#include <queue>
#include <chrono>

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

    static void print_metrics(const Metrics &m, const string &name)
    {
        cout << name <<": " << m.commands << " commands, " << m.blocks << " blocks" << endl;
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
        cout << "ctor Observer" << endl;
        run_flag = true;
    }

    ~Observer()
    {
        cout << "dtor Observer" << endl;
        run_flag = false;
    }

    virtual void dump(Commands &cmd) = 0;
    virtual void stop() = 0;

    atomic<bool> run_flag;
};

class Dumper
{
    vector<Observer *> subs;
public:
    Dumper();
    ~Dumper();
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
    ConsoleDumper(shared_ptr<Dumper> dmp);
    ~ConsoleDumper();
    void dump(Commands &cmd);
    void stop();
    Metrics dumper();
};

class FileDumper : public Observer
{
    mutex m;
    condition_variable cv;
    bool flag;
    queue<Commands> commands;

public:
    FileDumper(shared_ptr<Dumper> dmp);
    ~FileDumper();
    void dump(Commands &cmd);
    void stop();
    Metrics dumper();
    string get_unique_number();
};


class BulkContext
{
    static constexpr char delimiter = '\n';
    size_t bulk_size;
    shared_ptr<Dumper> dumper;

    Commands cmds;
    Metrics metrics;

    size_t lines_count;
    bool blockFound;
    int nestedBlocksCount;

    queue<Commands> *console_queue;
    queue<Commands> *file_queue;

    mutex *console_mutex;
    mutex *file_mutex;

    condition_variable *console_cv;
    condition_variable *file_cv;

public:
    shared_ptr<ConsoleDumper> conDumper;
    shared_ptr<FileDumper> fileDumper;

    BulkContext(size_t bulk_size);
    BulkContext(size_t bulk_size_, queue<Commands> *cq, queue<Commands> *fq,
                        mutex *cm, mutex *fm,
                        condition_variable *ccv, condition_variable *fcv);
    ~BulkContext();

    void add_line(string &cmd);
    void end_input();
    void print_metrics();
    void dump(Commands cmd);
};

}
