/**
 * @file FilterOrchestrator.hpp
 *
 * Developer(s) of this DAQ application have yet to replace this line with a
 * brief description of the application.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef DFBACKEND_INCLUDE_FILTERORCHESTRATOR_HPP_
#define DFBACKEND_INCLUDE_FILTERORCHESTRATOR_HPP_

#include <sys/wait.h>

#include <algorithm>
#include <execution>
#include <fstream>

#include "datafilter/datafilter_structs.hpp"
#include "iomanager/IOManager.hpp"
#include "logging/Logging.hpp"

using namespace dunedaq::iomanager;

namespace dunedaq {
namespace datafilter {

struct FilterOrchestratorConfig {
    bool use_connectivity_service = false;  // unsed for now
    int port = 5000;
    std::string server = "127.0.0.1";
    std::string server_trdispatcher = "127.0.0.1";

    std::string session_name = "FilterOrchestrator test run";
    size_t num_apps = 1;
    size_t num_connections_per_group = 1;
    size_t num_groups = 1;
    size_t num_messages = 1;
    size_t message_size_kb = 1024;
    size_t num_runs = 1;
    size_t my_id = 0;
    size_t send_interval_ms = 100;
    int publish_interval = 1000;
    bool next_tr = false;

    size_t seq_number;
    size_t trigger_number;
    size_t trigger_timestamp;
    size_t run_number;
    size_t element_id;
    size_t detector_id;
    size_t error_bits;

    void configure_connsvc() {
        setenv("CONNECTION_SERVER", server.c_str(), 1);
        setenv("CONNECTION_PORT", std::to_string(port).c_str(), 1);
    }

    std::string get_connection_name(size_t app_id, size_t group_id,
                                    size_t conn_id) {
        std::stringstream ss;
        ss << "conn_A" << app_id << "_G" << group_id << "_C" << conn_id << "_";
        return ss.str();
    }
    std::string get_group_connection_name(size_t app_id, size_t group_id) {
        std::stringstream ss;
        ss << "conn_A" << app_id << "_G" << group_id << "_.*";
        return ss.str();
    }

    std::string get_connection_ip(size_t app_id, size_t group_id,
                                  size_t conn_id) {
        assert(num_apps < 253);
        assert(num_groups < 253);
        assert(num_connections_per_group < 252);

        int first_byte = conn_id + 2;    // 2-254
        int second_byte = group_id + 1;  // 1-254
        int third_byte = app_id + 1;     // 1 - 254
        std::string conn_addr;

        if (server == "127.0.0.1") {
            conn_addr = "tcp://127." + std::to_string(third_byte) + "." +
                        std::to_string(second_byte) + "." +
                        std::to_string(first_byte) + ":" + std::to_string(port);
        } else {
            conn_addr = "tcp://" + server + ":" + std::to_string(port);
        }

        return conn_addr;
    }

    std::string get_pub_init_name() { return get_pub_init_name(my_id); }
    std::string get_pub_init_name(size_t id) {
        return "conn_init_" + std::to_string(id);
    }
    // std::string get_publisher_init_name() { return "conn_init_.*"; }

    void configure_iomanager() {
        setenv("DUNEDAQ_PARTITION", session_name.c_str(), 0);

        Queues_t queues;
        Connections_t connections;

        //      for (size_t group = 0; group < num_groups; ++group) {
        //        for (size_t conn = 0; conn < num_connections_per_group;
        //        ++conn) {
        //          auto conn_addr = get_connection_ip(my_id, group, conn);
        //          TLOG() << "Adding connection with id " <<
        //          get_connection_name(my_id, group, conn) << " and address "
        //                        << conn_addr;
        //
        //          connections.emplace_back(Connection{
        //            ConnectionId{ get_connection_name(my_id, group, conn),
        //            "data_t" }, conn_addr, ConnectionType::kPubSub });
        //        }
        //      }

        //      for (size_t sub = 0; sub < num_apps; ++sub) {
        for (size_t sub = 0; sub < 3; ++sub) {
            auto port = 13000 + sub;
            std::string conn_addr =
                "tcp://" + server + ":" + std::to_string(port);
            TLOG() << "Adding control connection "
                   << "TR_tracking" + std::to_string(sub) << " with address "
                   << conn_addr;

            connections.emplace_back(
                // Connection{ ConnectionId{ "TR_tracking"+std::to_string(sub),
                // "init_t" }, conn_addr, ConnectionType::kPubSub });
                Connection{
                    ConnectionId{"TR_tracking" + std::to_string(sub), "init_t"},
                    conn_addr, ConnectionType::kSendRecv});
        }

        //      for (size_t sub = 0; sub < num_apps; ++sub) {
        for (size_t sub = 0; sub < 3; ++sub) {
            auto port = 23000 + sub;
            std::string conn_addr =
                "tcp://" + server_trdispatcher + ":" + std::to_string(port);
            TLOG() << "Adding control connection "
                   << "trdispatcher" + std::to_string(sub) << " with address "
                   << conn_addr;

            connections.emplace_back(
                // Connection{ ConnectionId{ "TR_tracking"+std::to_string(sub),
                // "init_t" }, conn_addr, ConnectionType::kPubSub });
                Connection{ConnectionId{"trdispatcher" + std::to_string(sub),
                                        "init_t"},
                           conn_addr, ConnectionType::kSendRecv});
        }

        IOManager::get()->configure(
            queues, connections, use_connectivity_service,
            std::chrono::milliseconds(publish_interval));
    }
};

struct FilterOrchestrator {
    struct FilterOrchestratorInfo {
        size_t conn_id;
        size_t group_id;
        size_t messages_sent{0};
        size_t trigger_number;
        size_t trigger_timestamp;
        size_t run_number;
        size_t element_id;
        size_t detector_id;
        size_t error_bits;
        // dunedaq::daqdataformats::Fragment fragment_type;
        size_t fragment_type;
        std::string path_header;
        int n_frames;

        std::shared_ptr<SenderConcept<dunedaq::datafilter::Data>> sender;
        std::unique_ptr<std::thread> send_thread;
        std::chrono::milliseconds get_sender_time;

        FilterOrchestratorInfo(size_t group, size_t conn)
            : conn_id(conn), group_id(group) {}
    };

    std::vector<std::shared_ptr<FilterOrchestratorInfo>> filterorchestrators;
    FilterOrchestratorConfig config;

    uint16_t data3[200000000];
    uint32_t nchannels = 64;
    uint32_t nsamples = 64;

    std::string path_header1;

    explicit FilterOrchestrator(FilterOrchestratorConfig c) : config(c) {
        config.configure_iomanager();
    }
    ~FilterOrchestrator() { dunedaq::iomanager::IOManager::get()->reset(); }

    void init(size_t run_number) {
        TLOG_DEBUG(5) << "Getting init sender";
        // auto init_sender =
        // dunedaq::get_iom_sender<dunedaq::datafilter::Handshake>(config.get_pub_init_name());
        auto init_sender =
            dunedaq::get_iom_sender<dunedaq::datafilter::Handshake>(
                "TR_tracking0");
        auto init_receiver =
            dunedaq::get_iom_receiver<dunedaq::datafilter::Handshake>(
                "TR_tracking1");

        auto init_sender1 =
            dunedaq::get_iom_sender<dunedaq::datafilter::Handshake>(
                "trdispatcher0");
        auto init_receiver1 =
            dunedaq::get_iom_receiver<dunedaq::datafilter::Handshake>(
                "trdispatcher1");

        std::atomic<std::chrono::steady_clock::time_point> last_received =
            std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - last_received.load())
                   .count() < 500) {
            //    while (1) {

            // Handshake q(config.my_id, -1, 0, run_number);
            dunedaq::datafilter::Handshake q("start");
            init_sender->send(std::move(q), Sender::s_block);
            dunedaq::datafilter::Handshake recv;
            recv = init_receiver->receive(std::chrono::milliseconds(100));
            std::this_thread::sleep_for(100ms);
            if (recv.msg_id == "gotit") TLOG() << "Receiver got it";
            init_sender1->send(std::move(q), Sender::s_block);
            break;
        }
    }

    void send(size_t run_number, pid_t subscriber_pid) {
        std::ostringstream ss;
        auto init_receiver =
            dunedaq::get_iom_receiver<dunedaq::datafilter::Handshake>(
                "TR_tracking2");
        std::unordered_map<int, std::set<size_t>> completed_receiver_tracking;
        std::mutex tracking_mutex;

        //    for (size_t group = 0; group < config.num_groups; ++group) {
        //      for (size_t conn = 0; conn < config.num_connections_per_group;
        //      ++conn) {
        // auto info = std::make_shared<FilterOrchestratorInfo>(group, conn);
        auto info = std::make_shared<FilterOrchestratorInfo>(0, 0);
        filterorchestrators.push_back(info);
        //      }
        //    }

        TLOG_DEBUG(7) << "Getting publisher objects for each connection";
        std::for_each(
            std::execution::par_unseq, std::begin(filterorchestrators),
            std::end(filterorchestrators),
            [=](std::shared_ptr<FilterOrchestratorInfo> info) {
                auto before_sender = std::chrono::steady_clock::now();
                info->sender =
                    dunedaq::get_iom_sender<dunedaq::datafilter::Data>(
                        config.get_connection_name(config.my_id, info->group_id,
                                                   info->conn_id));
                auto after_sender = std::chrono::steady_clock::now();
                info->get_sender_time =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        after_sender - before_sender);
            });

        // auto size = 1024;

        TLOG_DEBUG(7) << "Starting publish threads";
        std::for_each(
            std::execution::par_unseq, std::begin(filterorchestrators),
            std::end(filterorchestrators),
            [=, &completed_receiver_tracking,
             &tracking_mutex](std::shared_ptr<FilterOrchestratorInfo> info) {
                info->send_thread.reset(new std::thread(
                    [=, &completed_receiver_tracking, &tracking_mutex]() {
                        bool complete_received = false;

                        while (!complete_received) {
                            // wait for the next TR request
                            std::atomic<std::chrono::steady_clock::time_point>
                                last_received =
                                    std::chrono::steady_clock::now();
                            while (std::chrono::duration_cast<
                                       std::chrono::milliseconds>(
                                       std::chrono::steady_clock::now() -
                                       last_received.load())
                                       .count() < 500) {
                                dunedaq::datafilter::Handshake recv;
                                recv =
                                    init_receiver->receive(Receiver::s_block);
                                TLOG() << "recv.msg_id " << recv.msg_id;
                                std::this_thread::sleep_for(100ms);
                                if (recv.msg_id == "wait") {
                                    //     if (config.next_tr) {
                                    auto next_tr_sender =
                                        dunedaq::get_iom_sender<
                                            dunedaq::datafilter::Handshake>(
                                            "trdispatcher2");
                                    TLOG() << "send wait for next instruction";
                                    dunedaq::datafilter::Handshake q("wait");
                                    next_tr_sender->send(std::move(q),
                                                         Sender::s_block);
                                    //      }
                                    continue;
                                } else if (recv.msg_id == "next_tr") {
                                    TLOG() << "Got next_tr instruction";
                                    //    if (config.next_tr) {
                                    auto next_tr_sender =
                                        dunedaq::get_iom_sender<
                                            dunedaq::datafilter::Handshake>(
                                            "trdispatcher2");
                                    TLOG() << "send next_tr instruction";
                                    dunedaq::datafilter::Handshake q("next_tr");
                                    next_tr_sender->send(std::move(q),
                                                         Sender::s_block);
                                    //    }
                                    break;
                                }
                            }

                            //}
                            // force the while loop to end when no trigger path
                            // left. complete_received = true;
                        }
                    }));
            });

        TLOG_DEBUG(7) << "Joining send threads";
        for (auto& sender : filterorchestrators) {
            sender->send_thread->join();
            sender->send_thread.reset(nullptr);
        }
    }

    void request_next_tr(size_t run_number, pid_t subscriber_pid) {
        bool handshake_done = false;

        std::atomic<unsigned int> sent_cnt = 0;

        auto sender_next_tr =
            dunedaq::get_iom_sender<dunedaq::datafilter::Handshake>(
                "trdispatcher0");

        // std::chrono::milliseconds timeout(500);
        dunedaq::datafilter::Handshake sent_t1("trdispatcher0");
        // sender_next_tr->send(std::move(sent_t1), timeout);
        sender_next_tr->send(std::move(sent_t1), Sender::s_block);
    }

    void receive(size_t dataflow_run_number1, pid_t subscriber_pid) {
        bool handshake_done = false;
        std::atomic<unsigned int> received_cnt = 0;

        auto cb_receiver =
            dunedaq::get_iom_receiver<dunedaq::datafilter::Handshake>(
                "trdispatcher1");
        std::function<void(dunedaq::datafilter::Handshake)> str_receiver_cb =
            [&](dunedaq::datafilter::Handshake msg) {
                if (msg.msg_id == "next_tr") {
                    ++received_cnt;
                }
                TLOG() << "Receive instruction from filter results writer : "
                       << msg.msg_id;
            };

        cb_receiver->add_callback(str_receiver_cb);
        while (!handshake_done) {
            if (received_cnt == 1) handshake_done = true;
        }

        cb_receiver->remove_callback();

        request_next_tr(dataflow_run_number1, subscriber_pid);
    }
};

}  // namespace datafilter
DUNE_DAQ_SERIALIZABLE(dunedaq::datafilter::Handshake, "init_t");
}  // namespace dunedaq

#endif  // DFBACKEND_INCLUDE_FILTERORCHESTRATOR_HPP_
