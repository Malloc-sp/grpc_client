#include <iostream>
#include <grpcpp/grpcpp.h>
#include "grpcdata.grpc.pb.h"
#include <thread>

using grpc::ClientContext;
using grpc::CreateChannel;
using grpc::InsecureChannelCredentials;
using elephant::cnc::net::v1::GrpcData;
using elephant::cnc::net::v1::SendMessageRequest;
using elephant::cnc::net::v1::SendMessageResponse;

std::string StatusCodeToString(grpc::StatusCode code) {
    switch (code) {
        case grpc::StatusCode::OK:                  return "OK";
        case grpc::StatusCode::CANCELLED:           return "CANCELLED";
        case grpc::StatusCode::UNKNOWN:             return "UNKNOWN";
        case grpc::StatusCode::INVALID_ARGUMENT:    return "INVALID_ARGUMENT";
        case grpc::StatusCode::DEADLINE_EXCEEDED:   return "DEADLINE_EXCEEDED";
        case grpc::StatusCode::NOT_FOUND:           return "NOT_FOUND";
        case grpc::StatusCode::ALREADY_EXISTS:      return "ALREADY_EXISTS";
        case grpc::StatusCode::PERMISSION_DENIED:   return "PERMISSION_DENIED";
        case grpc::StatusCode::UNAUTHENTICATED:     return "UNAUTHENTICATED";
        case grpc::StatusCode::RESOURCE_EXHAUSTED:  return "RESOURCE_EXHAUSTED";
        case grpc::StatusCode::FAILED_PRECONDITION: return "FAILED_PRECONDITION";
        case grpc::StatusCode::ABORTED:             return "ABORTED";
        case grpc::StatusCode::OUT_OF_RANGE:        return "OUT_OF_RANGE";
        case grpc::StatusCode::UNIMPLEMENTED:       return "UNIMPLEMENTED";
        case grpc::StatusCode::INTERNAL:            return "INTERNAL";
        case grpc::StatusCode::UNAVAILABLE:         return "UNAVAILABLE";
        case grpc::StatusCode::DATA_LOSS:           return "DATA_LOSS";
        default:                                    return "UNKNOWN_CODE";
    }
}

int main() {
    std::string ip = "192.168.1.20";
    std::string pin = "293061";
    std::string is_reconnect = "1";
    // std::cout << "小屏ip: ";
    // std::cin >> ip;
    // std::cout << "pin: ";
    // std::cin >> pin;
    // std::cout << "是否定性为重连? 0否, 1是:";
    // std::cin >> is_reconnect;
    // 1. 创建 insecure 连接
    auto channel = CreateChannel(ip + ":65002", InsecureChannelCredentials());
    auto stub = GrpcData::NewStub(channel);

    // 2. 准备上下文，添加 metadata（模拟 CNC 客户端）
    ClientContext ctx;
    ctx.AddMetadata("user-name", ip);
    ctx.AddMetadata("user-password", pin);
    ctx.AddMetadata("user-from", "PC");
    ctx.AddMetadata("is-reconnect", is_reconnect);

    // 3. 建立双向流
    auto stream = stub->sendMessage(&ctx);
    if (!stream) {
        std::cerr << "Stream creation failed\n";
        return 1;
    }

    // 4. 发送一条初始化消息（必须，使服务端进入 READ 状态）
    SendMessageRequest init;
    init.set_from("PC");
    init.set_to("CNC");
    init.set_content("");
    init.set_type(elephant::cnc::net::v1::ContentType::Text);
    init.set_timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    if (!stream->Write(init)) {
        std::cerr << "小屏幕未开启服务器\n";
        return 1;
    }

    // 5. 发送一条实际消息
    SendMessageRequest msg;
    msg.set_from("test_user_CNC");
    msg.set_to("test_user_PC");
    msg.set_content("Hello from CNC");
    msg.set_type(elephant::cnc::net::v1::ContentType::Text);
    msg.set_timestamp(init.timestamp() + 1);
    if (!stream->Write(msg)) {
        std::cerr << "Write message failed\n";
        return 1;
    }
    std::thread([&]{
        SendMessageResponse response;
        while (stream->Read(&response)) {
            std::cout << "Received: " << std::endl << 
            response.content() << std::endl << "from: " << response.from() << std::endl;
        }
    }).detach();
    // 6. 接收服务端的响应（等待一条消息）
    while(1)
    {
        // 5. 发送一条实际消息
        SendMessageRequest msg;
        msg.set_from("192.168.60.132_PC");
        msg.set_to("192.168.60.132_CNC");
        std::string str;
        std::cout << "请输入text: ";
        std::cin >> str;
        //str = "test";
        msg.set_content(str);
        msg.set_type(elephant::cnc::net::v1::ContentType::Text);
        msg.set_timestamp(init.timestamp() + 1);
        if (!stream->Write(msg)) {
            break;
        }
	else
	{
		std::cout << "send success" << std::endl;
	}
    }
    stream->WritesDone();
    auto status = stream->Finish();
    std::cout << StatusCodeToString(status.error_code()) << std::endl;
    return 0;
}
