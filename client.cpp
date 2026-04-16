#include <iostream>
#include <grpcpp/grpcpp.h>
#include "grpcdata.grpc.pb.h"
#include <thread>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <fstream>
#include <chrono>

using grpc::ClientContext;
using grpc::CreateChannel;
using grpc::InsecureChannelCredentials;
using elephant::cnc::net::v1::GrpcData;
using elephant::cnc::net::v1::SendMessageRequest;
using elephant::cnc::net::v1::SendMessageResponse;
using elephant::cnc::net::v1::SendImageRequest;
using elephant::cnc::net::v1::SendImageResponse;

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
    std::string ip = "192.168.7.42";
    auto channel = CreateChannel(ip + ":65002", InsecureChannelCredentials());
    auto stub = GrpcData::NewStub(channel);
    
    ClientContext msg_ctx;
    msg_ctx.AddMetadata("user-from", "PC");
    msg_ctx.AddMetadata("user-name", ip);
    msg_ctx.AddMetadata("user-password", "355793");
    //msg_ctx.AddMetadata("", "PC");
    auto msg_stream = stub->sendMessage(&msg_ctx);
    if (!msg_stream) {
        std::cerr << "sendMessage stream creation failed\n";
        //stream->WritesDone();
        //(void)stream->Finish();
        return 1;
    }
    printf("test\n");
    std::thread reader([&]() {
        SendMessageResponse resp;
        while (msg_stream->Read(&resp)) {
            if (resp.type() != elephant::cnc::net::v1::ContentType::Text) {
                //std::cout << "[recv non-Text type=" << resp.type() << "] size=" << resp.content().size() << "\n";
                continue;
            }
            const std::string text(resp.content().begin(), resp.content().end());
            // std::cout << "[recv] from=" << resp.from() << " to=" << resp.to() << " ts=" << resp.timestamp()
            //           << " text=" << text << "\n";
        }
    });

    std::cout << "Type messages to send. Type /quit to exit.\n";
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "/quit") break;
        if (line.empty()) continue;

        SendMessageRequest req;
        const auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now());
        req.set_timestamp(static_cast<uint64_t>(now.time_since_epoch().count()));
        req.set_type(elephant::cnc::net::v1::ContentType::Text);
        req.set_from("PC");
        req.set_to("192.168.7.42_CNC");
        req.set_content(line);

        if (!msg_stream->Write(req)) {
            std::cerr << "Write() failed (stream closed by server?)\n";
            break;
        }
    }

    msg_stream->WritesDone();
    if (reader.joinable()) reader.join();
    auto msg_status = msg_stream->Finish();
    std::cout << "sendMessage finished with status: " << StatusCodeToString(msg_status.error_code()) << "\n";

    // 关闭之前创建的 sendImage 流（本次不处理 Image）
    //stream->WritesDone();
    //auto img_status = stream->Finish();
    //std::cout << "sendImage finished with status: " << StatusCodeToString(img_status.error_code()) << "\n";

    cv::destroyAllWindows();
    return 0;
}