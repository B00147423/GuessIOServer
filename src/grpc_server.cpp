#include <grpcpp/grpcpp.h>
#include <iostream>

#include "../proto/proto_gen/guessio.grpc.pb.h"
#include "../proto/proto_gen/guessio.pb.h"

// Don’t use "using grpc::Server;" because you already have your own Server class
namespace g = grpc;  // alias grpc namespace for safety

using g::ServerBuilder;
using g::ServerContext;
using g::Status;

using guessio::GuessService;
using guessio::JoinRequest;
using guessio::JoinReply;
using guessio::GuessRequest;
using guessio::GuessReply;

class GuessServiceImpl final : public GuessService::Service {
public:
    Status JoinGame(g::ServerContext* context, const JoinRequest* request, JoinReply* reply) override {
        std::cout << "[gRPC] " << request->username() << " joined!" << std::endl;
        reply->set_message("Welcome " + request->username() + "!");
        return g::Status::OK;
    }

    Status MakeGuess(g::ServerContext* context, const GuessRequest* request, GuessReply* reply) override {
        std::cout << "[gRPC] " << request->username() << " guessed: " << request->guess() << std::endl;
        reply->set_correct(request->guess() == "apple");
        reply->set_hint("Try again!");
        return g::Status::OK;
    }
};

void StartGrpcServer() {
    std::string server_address("0.0.0.0:50051");
    GuessServiceImpl service;

    g::ServerBuilder builder;
    builder.AddListeningPort(server_address, g::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<g::Server> server(builder.BuildAndStart());
    std::cout << "[gRPC] Listening on " << server_address << std::endl;

    server->Wait();
}
