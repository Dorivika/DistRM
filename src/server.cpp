#include <grpcpp/grpcpp.h>
#include "proto/system_monitor.pb.h"
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

class SystemMonitorImpl final : public system_monitor::SystemMonitor::Service {
private:
    std::mutex mutex_;
    std::unordered_map<std::string, std::queue<system_monitor::Alert>> alerts_;
    
    double get_cpu_usage() {
        struct proc_stat stats;
        if (read_stat(&stats) == 0) {
            static unsigned long long prev_total = 0;
            static unsigned long long prev_idle = 0;
            
            unsigned long long total = stats.cpu.user + stats.cpu.nice + 
                                     stats.cpu.system + stats.cpu.idle + 
                                     stats.cpu.iowait + stats.cpu.irq + 
                                     stats.cpu.softirq + stats.cpu.steal;
            unsigned long long idle = stats.cpu.idle + stats.cpu.iowait;
            
            double cpu_usage = 0.0;
            if (prev_total > 0) {
                unsigned long long total_delta = total - prev_total;
                unsigned long long idle_delta = idle - prev_idle;
                cpu_usage = (1.0 - static_cast<double>(idle_delta) / total_delta) * 100.0;
            }
            
            prev_total = total;
            prev_idle = idle;
            return cpu_usage;
        }
        return 0.0;
    }

    unsigned long get_memory_usage() {
        struct meminfo mem_info;
        if (read_meminfo(&mem_info) == 0) {
            return mem_info.MemTotal - mem_info.MemFree - mem_info.Cached - mem_info.Buffers;
        }
        return 0;
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