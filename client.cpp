#include <iostream>
#include <grpcpp/grpcpp.h>
#include "grpcdata.grpc.pb.h"
#include <thread>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <fstream>

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
    std::string ip = "192.168.60.132";
    auto channel = CreateChannel(ip + ":65002", InsecureChannelCredentials());
    auto stub = GrpcData::NewStub(channel);
    ClientContext ctx;
    ctx.AddMetadata("user-from", "PC");
    auto stream = stub->sendImage(&ctx);  // 假设这是服务端主动推送的流
    if (!stream) {
        std::cerr << "Stream creation failed\n";
        return 1;
    }

    // 创建 OpenCV 窗口
    cv::namedWindow("Video Stream", cv::WINDOW_NORMAL);

    SendImageResponse response;
    while (stream->Read(&response)) {
        const std::string &jpeg_data = response.content();
        const std::string &name = response.cameratype();
        std::cout << name << std::endl;
        if (jpeg_data.empty()) continue;

        // 将 std::string 转换为 std::vector<char> 供 OpenCV 解码
        std::vector<char> img_vec(jpeg_data.begin(), jpeg_data.end());

        // 解码 JPEG -> cv::Mat
        cv::Mat frame = cv::imdecode(img_vec, cv::IMREAD_COLOR);
        if (frame.empty()) {
            std::cerr << "Failed to decode image" << std::endl;
            continue;
        }

        // 显示图像
        cv::imshow("Video Stream", frame);

        // 按 'q' 键退出
        if (cv::waitKey(1) == 'q') break;
    }

    // 流结束或出错
    stream->WritesDone();
    auto status = stream->Finish();
    std::cout << "Stream finished with status: " << StatusCodeToString(status.error_code()) << std::endl;
    cv::destroyAllWindows();
    return 0;
}