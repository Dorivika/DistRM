#include <grpcpp/grpcpp.h>
#include "proto/system_monitor.pb.h"
#include <sys/sysinfo.h>
#include <proc/readproc.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>
#include <mutex>
#include <queue>

class SystemMonitorImpl final : public system_monitor::SystemMonitor::Service {
private:
    std::mutex mutex_;
    std::unordered_map<std::string, std::queue<system_monitor::Alert>> alerts_;
    
    double get_cpu_usage() {
        // Implementation using /proc/stat
        static unsigned long long prev_total_time = 0;
        static unsigned long long prev_idle_time = 0;
        
        std::ifstream proc_stat("/proc/stat");
        std::string cpu_label;
        unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
        
        proc_stat >> cpu_label >> user >> nice >> system >> idle 
                 >> iowait >> irq >> softirq >> steal;
        
        unsigned long long total_time = user + nice + system + idle + iowait + irq + softirq + steal;
        unsigned long long current_idle_time = idle + iowait;
        
        double cpu_usage = 0.0;
        if (prev_total_time != 0) {
            unsigned long long total_delta = total_time - prev_total_time;
            unsigned long long idle_delta = current_idle_time - prev_idle_time;
            cpu_usage = (1.0 - static_cast<double>(idle_delta) / total_delta) * 100.0;
        }
        
        prev_total_time = total_time;
        prev_idle_time = current_idle_time;
        
        return cpu_usage;
    }

public:
    grpc::Status GetSystemStats(grpc::ServerContext* context,
                              const system_monitor::SystemRequest* request,
                              system_monitor::SystemStats* response) override {
        response->set_cpu_usage(get_cpu_usage());
        
        struct sysinfo si;
        if (sysinfo(&si) == 0) {
            double total_ram = si.totalram * si.mem_unit;
            double free_ram = si.freeram * si.mem_unit;
            response->set_memory_usage(((total_ram - free_ram) / total_ram) * 100.0);
        }
        
        // Add disk stats
        for (const auto& entry : std::filesystem::space_info("/")) {
            auto* disk_stat = response->add_disk_stats();
            disk_stat->set_mount_point("/");
            disk_stat->set_total_space(entry.capacity);
            disk_stat->set_used_space(entry.capacity - entry.free);
        }
        
        // Add process information using procps
        PROCTAB* proc = openproc(PROC_FILLMEM | PROC_FILLSTAT | PROC_FILLSTATUS);
        proc_t proc_info;
        memset(&proc_info, 0, sizeof(proc_info));
        
        while (readproc(proc, &proc_info) != nullptr) {
            auto* process = response->add_processes();
            process->set_pid(proc_info.tid);
            process->set_name(proc_info.cmd);
            process->set_cpu_usage(proc_info.pcpu);
            process->set_memory_usage((proc_info.vm_rss * 100.0) / si.totalram);
        }
        closeproc(proc);
        
        return grpc::Status::OK;
    }

    grpc::Status StreamResourceUsage(grpc::ServerContext* context,
                                   const system_monitor::SystemRequest* request,
                                   grpc::ServerWriter<system_monitor::ResourceUpdate>* writer) override {
        system_monitor::ResourceUpdate update;
        
        while (!context->IsCancelled()) {
            update.set_cpu_usage(get_cpu_usage());
            
            struct sysinfo si;
            if (sysinfo(&si) == 0) {
                double total_ram = si.totalram * si.mem_unit;
                double free_ram = si.freeram * si.mem_unit;
                update.set_memory_usage(((total_ram - free_ram) / total_ram) * 100.0);
            }
            
            auto now = std::chrono::system_clock::now();
            update.set_timestamp(std::to_string(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()
                ).count()
            ));
            
            writer->Write(update);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        return grpc::Status::OK;
    }
};

int main(int argc, char** argv) {
    std::string server_address("0.0.0.0:50051");
    SystemMonitorImpl service;
    
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
    
    return 0;
}