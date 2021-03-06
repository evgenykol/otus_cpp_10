#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <memory>

namespace bulk{

class Metrics
{
public:
    size_t blocks;
    size_t commands;

    Metrics() : blocks(0), commands(0) {}
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

    static void print_metrics(const Metrics &m, const std::string &name)
    {
        std::cout << name << ": " << m.commands << " commands, " << m.blocks << " blocks" << std::endl;
    }
};

class Commands
{
public:
    std::vector<std::string> cmds;
    time_t timestamp;
    Metrics metrics;

    void push_back(std::string str);
    void push_back_block(std::string str);
    void clear();
};

class Observer
{
protected:
    std::mutex m;
    std::condition_variable cv;
    std::queue<Commands> q;
    std::atomic<bool> run_flag;

public:
    Observer();
    ~Observer();

    virtual void dump(Commands &cmd) = 0;
    virtual void stop() = 0;
    virtual void dumper(Metrics &metrics) = 0;

    bool queue_not_empty();
};

class Dumper
{
    std::vector<Observer *> subs;
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
public:
    ConsoleDumper(std::shared_ptr<Dumper> dmp);
    ~ConsoleDumper();
    void dump(Commands &cmd);
    void stop();
    void dumper(Metrics &metrics);

};

class FileDumper : public Observer
{
    std::atomic<int> unique_file_counter;
public:
    FileDumper(std::shared_ptr<Dumper> dmp);
    ~FileDumper();
    void dump(Commands &cmd);
    void stop();
    void dumper(Metrics &metrics);

    std::string get_unique_number();
};


class BulkContext
{
    static constexpr char delimiter = '\n';
    size_t bulk_size;
    std::shared_ptr<Dumper> dumper;

    Commands cmds;
    Metrics metrics;

    size_t lines_count;
    bool blockFound;
    int nestedBlocksCount;

    void dump_block();

public:
    std::shared_ptr<ConsoleDumper> conDumper;
    std::shared_ptr<FileDumper> fileDumper;

    BulkContext(size_t bulk_size_);
    ~BulkContext();

    void add_line(std::string &cmd);
    void end_input();
    void print_metrics();
};

}
