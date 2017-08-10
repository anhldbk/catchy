//#include <uvw.hpp>
//#include <memory>
//#include <iostream>
//using namespace std;
//const char PIPE_NAME[] = "evolas";
//
//void listen(uvw::Loop &loop) {
//    std::shared_ptr<uvw::PipeHandle> pipe = loop.resource<uvw::PipeHandle>();
//
//    pipe->once<uvw::ListenEvent>([](const uvw::ListenEvent &, uvw::PipeHandle &srv) {
//        std::shared_ptr<uvw::PipeHandle> client = srv.loop().resource<uvw::PipeHandle>();
//
//        client->on<uvw::CloseEvent>([ptr = srv.shared_from_this()](const uvw::CloseEvent &, uvw::PipeHandle &) { ptr->close(); });
//        client->on<uvw::EndEvent>([](const uvw::EndEvent &, uvw::PipeHandle &client) { client.close(); });
//        client->on<uvw::DataEvent>([](const uvw::DataEvent &, uvw::PipeHandle &client) { 
//            cout << "on data\n";
//        });
//        
//        srv.accept(*client);
//        client->read();
//    });
//
//    pipe->bind(PIPE_NAME);
//    pipe->listen();
//}
//
//void conn(uvw::Loop &loop) {
//    auto pipe = loop.resource<uvw::PipeHandle>();
//
//    pipe->on<uvw::ErrorEvent>([](const uvw::ErrorEvent &, uvw::PipeHandle &) { /* handle errors */ });
//
//    pipe->once<uvw::ConnectEvent>([](const uvw::ConnectEvent &, uvw::PipeHandle &pipe) {
//        auto dataWrite = std::unique_ptr<char[]>(new char[2]{ 'b', 'c' });
//        pipe.write(std::move(dataWrite), 2);
//        pipe.close();
//    });
//
//    pipe->connect(PIPE_NAME);
//}
//
//int main() {
//    auto loop = uvw::Loop::getDefault();
//    listen(*loop);
//    conn(*loop);
//    loop->run();
//}

#include <iostream>
#include <atomic>
#include <thread>
#include <string.h>
//#include <uvw/emitter.hpp>
#include <uvw.hpp>
#include <iostream>
using namespace std;

struct RequestEvent {
    int data;

    RequestEvent(int data) {
        this->data = data;
    }
};

struct ResponseEvent {
    int data;

    ResponseEvent(int data) {
        this->data = data;
    }
};

struct DataEmitter : uvw::Emitter<DataEmitter> {

    void request(int content) {
        publish(RequestEvent(content));
    }

    void response(int content) {
        publish(ResponseEvent(content));
    }

    DataEmitter() {

    }
};

int main() {

    auto emitter = make_shared<DataEmitter>();

    auto callback1 = [](const shared_ptr<DataEmitter>& emitter_ptr) {
        emitter_ptr->on<RequestEvent>([&](const RequestEvent& event, auto& emitter) {
            cout << "!> Serving request..." << event.data << endl;
            emitter_ptr->response(event.data + 1);
        });
    };

    auto callback2 = [](const shared_ptr<DataEmitter>& emitter_ptr) {
        auto loop = uvw::Loop::getDefault();
        
        emitter_ptr->on<ResponseEvent>([&](const ResponseEvent& event, auto& emitter) {
            cout << "!> Receiving response..." << event.data << endl;
        });
        
        std::shared_ptr<uvw::TimerHandle> timer = loop->resource<uvw::TimerHandle>();
        timer->on<uvw::TimerEvent>([&](const uvw::TimerEvent& event, uvw::TimerHandle& th) {
            std::shared_ptr<int> data = th.data<int>();
            emitter_ptr->request(*data);
            *data = *data + 1;
        });
        timer->data(std::make_shared<int>(0));

        timer->start(uvw::TimerHandle::Time{0}, uvw::TimerHandle::Time{10});
        loop->run();
    };


    thread t1(callback1, emitter), t2(callback2, emitter);
    t1.join();
    t2.join();

    cin;

}
