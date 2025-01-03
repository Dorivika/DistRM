// #pragma once
// #include <grpcpp/security/server_credentials.h>
// #include <grpcpp/security/auth_metadata_processor.h>

// class AuthMetadataProcessor : public grpc::AuthMetadataProcessor {
// public:
//     grpc::Status Process(const InputMetadata& auth_metadata,
//                         grpc::AuthContext* context,
//                         OutputMetadata* consumed_auth_metadata,
//                         OutputMetadata* response_metadata) override {
//         auto token_iter = auth_metadata.find("authorization");
//         if (token_iter == auth_metadata.end()) {
//             return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "No authorization token");
//         }
        
//         // Validate JWT token
//         if (!validate_jwt_token(token_iter->second)) {
//             return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid token");
//         }
        
//         consumed_auth_metadata->insert(std::make_pair("authorization", token_iter->second));
//         return grpc::Status::OK;
//     }

// private:
//     bool validate_jwt_token(const std::string& token) {
//         // JWT validation logic here
//         return true; // Simplified for example
//     }
// };

// // Modified server.cpp main() function
// int main(int argc, char** argv) {
//     std::string server_address("0.0.0.0:50051");
//     SystemMonitorImpl service;
    
//     // Set up SSL/TLS credentials
//     grpc::SslServerCredentialsOptions ssl_opts;
//     ssl_opts.pem_root_certs = read_file("ca.crt");
//     ssl_opts.pem_private_key = read_file("server.key");
//     ssl_opts.pem_cert_chain = read_file("server.crt");
    
//     auto server_creds = grpc::SslServerCredentials(ssl_opts);
    
//     // Set up authentication
//     auto auth_processor = std::make_shared<AuthMetadataProcessor>();
//     server_creds->SetAuthMetadataProcessor(auth_processor);
    
//     grpc::ServerBuilder builder;
//     builder.AddListeningPort(server_address, server_creds);
//     builder.RegisterService(&service);
    
//     // Enable logging
//     builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 10000);
//     builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 5000);
    
//     std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
//     std::cout << "Server listening on " << server_address << std::endl;
//     server->Wait();
    
//     return 0;
// }