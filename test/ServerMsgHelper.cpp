#include "common/common.hpp"
#include "distplusplus.grpc.pb.h"
#include "distplusplus.pb.h"

#include <cstdlib>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <grpc/grpc.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <iostream>
#include <memory>
#include <string>

int main(int argc, char *argv[]) {
	if (argc != 3) {
		std::cerr << "ServerMsgHelper: incorrect usage: expected 2 args but got " << argc - 1 << std::endl;
		return 1;
	}
	distplusplus::common::BoundsSpan args(argv, argc);
	std::string messageType = args[1];
	std::string messageContent = args[2];

	std::unique_ptr<google::protobuf::Message> message = nullptr;
	std::unique_ptr<google::protobuf::Message> answer = nullptr;
	if (messageType == "ServerQuery") {
		message = std::make_unique<distplusplus::ServerQuery>();
		answer = std::make_unique<distplusplus::QueryAnswer>();
	} else if (messageType == "Reservation") {
		message = std::make_unique<distplusplus::Reservation>();
		answer = std::make_unique<distplusplus::ReservationAnswer>();
	} else if (messageType == "CompileRequest") {
		message = std::make_unique<distplusplus::CompileRequest>();
		answer = std::make_unique<distplusplus::CompileAnswer>();
	} else {
		std::cerr << "ServerMsgHelper: unrecognized message type " << messageType << std::endl;
		return 1;
	}

	std::cerr << "ServerMsgHelper: received input " << messageContent << std::endl;
	google::protobuf::util::JsonStringToMessage(messageContent, message.get());
	std::string json;
	google::protobuf::util::MessageToJsonString(*message, &json);
	std::cerr << "ServerMsgHelper: parsed json as " << json << std::endl;

	const char *server = getenv("DISTPLUSPLUS_LISTEN_ADDRESS");
	if (server == nullptr) {
		std::cerr << "ServerMsgHelper: DISTPLUSPLUS_LISTEN_ADDRESS not set!" << std::endl;
		return 1;
	}
	grpc::ChannelArguments channelArgs;
	channelArgs.SetMaxReceiveMessageSize(INT_MAX);
	std::unique_ptr<distplusplus::CompilationServer::Stub> stub =
		distplusplus::CompilationServer::NewStub(grpc::CreateCustomChannel(server, grpc::InsecureChannelCredentials(), channelArgs));
	grpc::ClientContext context;
	grpc::Status ret;
	if (messageType == "ServerQuery") {
		ret = stub->Query(&context, *dynamic_cast<distplusplus::ServerQuery *>(message.get()),
						  dynamic_cast<distplusplus::QueryAnswer *>(answer.get()));
	} else if (messageType == "Reservation") {
		ret = stub->Reserve(&context, *dynamic_cast<distplusplus::Reservation *>(message.get()),
							dynamic_cast<distplusplus::ReservationAnswer *>(answer.get()));
		std::cout << dynamic_cast<distplusplus::ReservationAnswer *>(answer.get())->uuid();
	} else if (messageType == "CompileRequest") {
		ret = stub->Distribute(&context, *dynamic_cast<distplusplus::CompileRequest *>(message.get()),
							   dynamic_cast<distplusplus::CompileAnswer *>(answer.get()));
	}
	if (ret.error_code() != grpc::StatusCode::OK) {
		std::cout << distplusplus::common::mapGRPCStatus(ret.error_code());
		std::cerr << "ServerMsgHelper: rpc call " << messageType << " returned " << distplusplus::common::mapGRPCStatus(ret.error_code())
				  << " " << ret.error_message() << std::endl;
		return 1;
	}
}
