#include "client.hpp"

#include "common/common.hpp"
#include "config.hpp"
#include "fallback.hpp"

#include <boost/log/trivial.hpp>
#include <chrono>
#include <climits>
#include <grpc/grpc.h>
#include <grpcpp/create_channel.h>
#include <thread>

using namespace distplusplus;

namespace distplusplus::client {

grpc::Status Client::reserve() {
    grpc::ClientContext context;
    Reservation reservation;
    ReservationAnswer reservationAnswer;
    grpc::Status ret = stub->Reserve(&context, reservation, &reservationAnswer);
    if (reservationAnswer.success()) {
        if (!reservationAnswer.has_uuid()) {
            BOOST_LOG_TRIVIAL(warning)
                << "host " << context.peer() << " responded to reservation but with no UUID";
            throw FallbackSignal();
        }
        uuid = reservationAnswer.uuid();
    }
    return ret;
}

QueryAnswer Client::query(const ServerQuery &serverQuery) {
    grpc::ClientContext context;
    QueryAnswer queryAnswer;
    stub->Query(&context, serverQuery, &queryAnswer);
    return queryAnswer;
}

CompileAnswer Client::send(const std::string &compilerName, ArgsVecSpan args, const std::string &fileName,
                           const std::string &fileContent, const std::string &cwd) {

    // TODO: query

    // TODO: am I happy with this?
    bool foundServer = false;
    bool allDown = true;
    const std::chrono::system_clock::time_point timeBeforeReservation = std::chrono::system_clock::now();
    while (true) {
        for (const auto &server : config.servers()) {
            grpc::ChannelArguments channelArgs;
            channelArgs.SetMaxReceiveMessageSize(INT_MAX);
            stub = CompilationServer::NewStub(
                grpc::CreateCustomChannel(server, grpc::InsecureChannelCredentials(), channelArgs));
            const grpc::StatusCode status = reserve().error_code();
            if (status == grpc::StatusCode::OK) {
                allDown = false;
                foundServer = true;
                break;
            }
            if (status == grpc::StatusCode::UNAVAILABLE) {
                BOOST_LOG_TRIVIAL(info) << "host " << server << " is unreachable, attempting next host";
            } else {
                allDown = false;
                BOOST_LOG_TRIVIAL(warning)
                    << "host " << server << " replied with grpc error " << common::mapGRPCStatus(status)
                    << " to reservation, attempting next host";
            }
        }
        if (allDown) {
            const std::string err_msg("all specified hosts were unreachable");
            BOOST_LOG_TRIVIAL(fatal) << err_msg;
            throw std::runtime_error(err_msg);
        }
        if (foundServer) {
            break;
        }
        if (std::chrono::system_clock::now() - timeBeforeReservation >=
            std::chrono::seconds(config.reservationAttemptTimeout())) {
            const std::string err_msg("could not find a vacant host after " +
                                      std::to_string(config.reservationAttemptTimeout()) +
                                      " seconds, aborting");
            BOOST_LOG_TRIVIAL(fatal) << err_msg;
            throw std::runtime_error(err_msg);
        }
    }

    CompileRequest compileRequest;
    compileRequest.set_uuid(uuid);
    compileRequest.set_compiler(compilerName);
    for (const auto &arg : args) {
        compileRequest.add_argument(arg);
    }
    compileRequest.mutable_inputfile()->set_name(fileName);
    compileRequest.mutable_inputfile()->set_content(fileContent);
    // TODO: compression
    compileRequest.mutable_inputfile()->set_compressiontype(CompressionType::NONE);
    compileRequest.set_cwd(cwd);
    CompileAnswer compileAnswer;

    grpc::ClientContext context;

    const grpc::Status status = stub->Distribute(&context, compileRequest, &compileAnswer);
    if (status.error_code() == grpc::StatusCode::OK) {
        return compileAnswer;
    }
    // TODO: configure if should fallback
    BOOST_LOG_TRIVIAL(error) << "distributing to " << context.peer() << " failed with gRPC error "
                             << common::mapGRPCStatus(status.error_code()) << " " << status.error_message();
    throw FallbackSignal();
}

} // namespace distplusplus::client
