#include <grpcpp/grpcpp.h>
#include "proto/system_monitor.pb.h"
#include "proto/system_monitor.grpc.pb.h"
#include <sys/sysinfo.h>
#include <libproc2/stat.h>
#include <libproc2/meminfo.h>
#include <libproc2/pids.h>
#include <libproc2/diskstats.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <iostream>
#include <stdexcept>

// namespace proc {
//     struct cpu_stat_t {
//         unsigned long long user;
//         unsigned long long nice;
//         unsigned long long system;
//         unsigned long long idle;
//         unsigned long long iowait;
//         unsigned long long irq;
//         unsigned long long softirq;
//         unsigned long long steal;
//     };

//     struct proc_stat {
//         cpu_stat_t cpu;
//     };
// }

class SystemMonitorImpl final : public system_monitor::SystemMonitor::Service {
private:
    std::mutex mutex_;
    std::unordered_map<std::string, std::queue<system_monitor::Alert>> alerts_;
    
    double get_cpu_usage() {
        struct stat_info *stats = nullptr;
        if (procps_stat_new(&stats) == 0) {
            static unsigned long long prev_total = 0;
            static unsigned long long prev_idle = 0;
            
            unsigned long long user = STAT_GET(stats, STAT_TIC_USER, ull_int);
            unsigned long long nice = STAT_GET(stats, STAT_TIC_NICE, ull_int);
            unsigned long long system = STAT_GET(stats, STAT_TIC_SYSTEM, ull_int);
            unsigned long long idle = STAT_GET(stats, STAT_TIC_IDLE, ull_int);
            unsigned long long iowait = STAT_GET(stats, STAT_TIC_IOWAIT, ull_int);
            unsigned long long irq = STAT_GET(stats, STAT_TIC_IRQ, ull_int);
            unsigned long long softirq = STAT_GET(stats, STAT_TIC_SOFTIRQ, ull_int);
            unsigned long long stolen = STAT_GET(stats, STAT_TIC_STOLEN, ull_int);
            
            unsigned long long total = user + nice + system + idle + iowait + irq + softirq + stolen;
            unsigned long long total_idle = idle + iowait;
            
            double cpu_usage = 0.0;
            if (prev_total > 0) {
                unsigned long long total_delta = total - prev_total;
                unsigned long long idle_delta = total_idle - prev_idle;
                cpu_usage = (1.0 - static_cast<double>(idle_delta) / total_delta) * 100.0;
            }
            
            prev_total = total;
            prev_idle = total_idle;
            
            procps_stat_unref(&stats);
            return cpu_usage;
        }
        return 0.0;
    }

    unsigned long get_memory_usage() {
    struct meminfo_info *mem = nullptr;
    unsigned long used_memory = 0;

    if (procps_meminfo_new(&mem) == 0) {
        unsigned long total = MEMINFO_GET(mem, MEMINFO_MEM_TOTAL, ul_int);
        unsigned long free = MEMINFO_GET(mem, MEMINFO_MEM_FREE, ul_int);
        unsigned long cached = MEMINFO_GET(mem, MEMINFO_MEM_CACHED, ul_int);
        unsigned long buffers = MEMINFO_GET(mem, MEMINFO_MEM_BUFFERS, ul_int);
        
        used_memory = total - free - cached - buffers;
        
        procps_meminfo_unref(&mem);
    }
    return used_memory;
    }

    std::vector<pid_t> get_process_list() {
        std::vector<pid_t> pids;
        struct pids_info *info = NULL;
        struct pids_stack *stack = NULL;
        
        if (procps_pids_new(&info, NULL, 0) == 0) {
            while (procps_pids_stack_get(info, &stack) == 0) {
                pids.push_back(stack->tid);
            }
            procps_pids_unref(&info);
        }
        return pids;
    }

public:
    grpc::Status MonitorSystem(grpc::ServerContext* context,
                             const system_monitor::MonitorRequest* request,
                             system_monitor::MonitorResponse* response) override {
        response->set_cpu_usage(get_cpu_usage());
        response->set_memory_usage(get_memory_usage());
        
        auto pids = get_process_list();
        response->set_process_count(pids.size());
        
        return grpc::Status::OK;
    }
};

int main() {
    try {
        std::string server_address("0.0.0.0:50051");
        SystemMonitorImpl service;

        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        
        std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
        std::cout << "Server listening on " << server_address << std::endl;
        server->Wait();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}