// #include <grpcpp/grpcpp.h>
// #include <grpcpp/security/credentials.h>
// #include "proto/system_monitor.grpc.pb.h"
// #include <ncurses.h>
// #include <panel.h>
// #include <thread>
// #include <atomic>
// #include <fmt/format.h>
// #include <signal.h>
// #include <openssl/ssl.h>

// class SystemMonitorClient {
// private:
//     std::unique_ptr<system_monitor::SystemMonitor::Stub> stub_;
//     std::atomic<bool> running_{true};
//     std::vector<WINDOW*> windows_;
//     std::mutex data_mutex_;
//     system_monitor::SystemStats latest_stats_;
    
//     struct Credentials {
//         std::string client_cert;
//         std::string client_key;
//         std::string ca_cert;
//     };

//     // TLS setup
//     std::shared_ptr<grpc::ChannelCredentials> setup_secure_channel(const Credentials& creds) {
//         grpc::SslCredentialsOptions ssl_opts;
//         ssl_opts.pem_root_certs = creds.ca_cert;
//         ssl_opts.pem_private_key = creds.client_key;
//         ssl_opts.pem_cert_chain = creds.client_cert;
//         return grpc::SslCredentials(ssl_opts);
//     }

//     // UI Components
//     void init_ui() {
//         initscr();
//         start_color();
//         cbreak();
//         noecho();
//         keypad(stdscr, TRUE);
//         curs_set(0);
        
//         // Color pairs
//         init_pair(1, COLOR_GREEN, COLOR_BLACK);  // Normal
//         init_pair(2, COLOR_YELLOW, COLOR_BLACK); // Warning
//         init_pair(3, COLOR_RED, COLOR_BLACK);    // Critical
        
//         // Create windows
//         int max_y, max_x;
//         getmaxyx(stdscr, max_y, max_x);
        
//         // CPU Window
//         WINDOW* cpu_win = newwin(5, max_x/2, 0, 0);
//         windows_.push_back(cpu_win);
        
//         // Memory Window
//         WINDOW* mem_win = newwin(5, max_x/2, 0, max_x/2);
//         windows_.push_back(mem_win);
        
//         // Process Window
//         WINDOW* proc_win = newwin(max_y-5, max_x, 5, 0);
//         windows_.push_back(proc_win);
        
//         refresh();
//     }

//     void update_ui() {
//         std::lock_guard<std::mutex> lock(data_mutex_);
        
//         // Update CPU Window
//         WINDOW* cpu_win = windows_[0];
//         wclear(cpu_win);
//         box(cpu_win, 0, 0);
//         mvwprintw(cpu_win, 1, 2, "CPU Usage: %.2f%%", latest_stats_.cpu_usage());
//         wattron(cpu_win, COLOR_PAIR(get_color_pair(latest_stats_.cpu_usage())));
//         mvwprintw(cpu_win, 2, 2, "%s", create_bar(latest_stats_.cpu_usage()).c_str());
//         wattroff(cpu_win, COLOR_PAIR(get_color_pair(latest_stats_.cpu_usage())));
//         wrefresh(cpu_win);
        
//         // Update Memory Window
//         WINDOW* mem_win = windows_[1];
//         wclear(mem_win);
//         box(mem_win, 0, 0);
//         mvwprintw(mem_win, 1, 2, "Memory Usage: %.2f%%", latest_stats_.memory_usage());
//         wattron(mem_win, COLOR_PAIR(get_color_pair(latest_stats_.memory_usage())));
//         mvwprintw(mem_win, 2, 2, "%s", create_bar(latest_stats_.memory_usage()).c_str());
//         wattroff(mem_win, COLOR_PAIR(get_color_pair(latest_stats_.memory_usage())));
//         wrefresh(mem_win);
        
//         // Update Process Window
//         WINDOW* proc_win = windows_[2];
//         wclear(proc_win);
//         box(proc_win, 0, 0);
//         mvwprintw(proc_win, 1, 2, "PID\tNAME\t\tCPU%%\tMEM%%");
//         int row = 2;
//         for (const auto& proc : latest_stats_.processes()) {
//             mvwprintw(proc_win, row++, 2, "%d\t%s\t\t%.1f\t%.1f",
//                      proc.pid(), proc.name().c_str(),
//                      proc.cpu_usage(), proc.memory_usage());
//         }
//         wrefresh(proc_win);
//     }

//     int get_color_pair(double value) {
//         if (value > 90) return 3;      // Critical
//         else if (value > 70) return 2;  // Warning
//         return 1;                       // Normal
//     }

//     std::string create_bar(double percentage) {
//         const int width = 50;
//         int filled = static_cast<int>((percentage / 100.0) * width);
//         return std::string(filled, '|') + std::string(width - filled, ' ');
//     }

// public:
//     SystemMonitorClient(const std::string& address, const Credentials& creds) {
//         auto channel = grpc::CreateChannel(address, setup_secure_channel(creds));
//         stub_ = system_monitor::SystemMonitor::NewStub(channel);
//         init_ui();
//     }

//     ~SystemMonitorClient() {
//         running_ = false;
//         for (auto* win : windows_) {
//             delwin(win);
//         }
//         endwin();
//     }

//     void Run() {
//         // Start the monitoring threads
//         std::thread stats_thread(&SystemMonitorClient::MonitorStats, this);
//         std::thread alert_thread(&SystemMonitorClient::MonitorAlerts, this);
        
//         // Main UI loop
//         while (running_) {
//             update_ui();
//             std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
//             // Handle user input
//             int ch = getch();
//             if (ch == 'q' || ch == 'Q') {
//                 running_ = false;
//             }
//         }
        
//         stats_thread.join();
//         alert_thread.join();
//     }

// private:
//     void MonitorStats() {
//         system_monitor::SystemRequest request;
//         request.set_host_id("localhost");
        
//         while (running_) {
//             grpc::ClientContext context;
//             system_monitor::SystemStats stats;
            
//             auto status = stub_->GetSystemStats(&context, request, &stats);
            
//             if (status.ok()) {
//                 std::lock_guard<std::mutex> lock(data_mutex_);
//                 latest_stats_ = stats;
//             }
            
//             std::this_thread::sleep_for(std::chrono::seconds(1));
//         }
//     }

//     void MonitorAlerts() {
//         system_monitor::ThresholdRequest request;
//         request.set_host_id("localhost");
//         request.set_cpu_threshold(80.0);
//         request.set_memory_threshold(80.0);
        
//         while (running_) {
//             grpc::ClientContext context;
//             system_monitor::Alert alert;
            
//             auto reader = stub_->AlertOnThreshold(&context, request);
//             while (reader->Read(&alert)) {
//                 // Log alert to file
//                 log_alert(alert);
                
//                 // Display alert in UI
//                 display_alert(alert);
//             }
//         }
//     }

//     void log_alert(const system_monitor::Alert& alert) {
//         std::ofstream log_file("alerts.log", std::ios::app);
//         log_file << fmt::format("[{}] {} Alert: Current value: {:.2f}, Threshold: {:.2f}\n",
//                                alert.timestamp(),
//                                alert.resource_type(),
//                                alert.current_value(),
//                                alert.threshold());
//     }

//     void display_alert(const system_monitor::Alert& alert) {
//         // Flash warning on screen
//         flash();
//         beep();
        
//         // Create popup window for alert
//         int max_y, max_x;
//         getmaxyx(stdscr, max_y, max_x);
        
//         WINDOW* alert_win = newwin(5, 50, max_y/2-2, max_x/2-25);
//         box(alert_win, 0, 0);
//         wattron(alert_win, COLOR_PAIR(3) | A_BOLD);
//         mvwprintw(alert_win, 1, 2, "ALERT: %s", alert.resource_type().c_str());
//         mvwprintw(alert_win, 2, 2, "Current: %.2f%% Threshold: %.2f%%",
//                  alert.current_value(), alert.threshold());
//         wattroff(alert_win, COLOR_PAIR(3) | A_BOLD);
//         wrefresh(alert_win);
        
//         // Delete window after 3 seconds
//         std::thread([alert_win]() {
//             std::this_thread::sleep_for(std::chrono::seconds(3));
//             delwin(alert_win);
//         }).detach();
//     }
// };

// int main(int argc, char** argv) {
//     // Load certificates and keys
//     SystemMonitorClient::Credentials creds;
//     creds.client_cert = read_file("client.crt");
//     creds.client_key = read_file("client.key");
//     creds.ca_cert = read_file("ca.crt");
    
//     SystemMonitorClient client("localhost:50051", creds);
//     client.Run();
//     return 0;
// }