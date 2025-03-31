/**
 * @file FilterResultWriter.hpp
 *
 * Developer(s) of this DAQ application have yet to replace this line with a
 * brief description of the application.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef DFBACKEND_INCLUDE_FILTERRESULTWRITER_HPP_
#define DFBACKEND_INCLUDE_FILTERRESULTWRITER_HPP_

#include <sys/wait.h>

#include <algorithm>
#include <execution>
#include <fstream>

#include "datafilter/datafilter_structs.hpp"
#include "dfbackend/filterresultwriter/Nljs.hpp"
#include "dfbackend/filterresultwriter/Structs.hpp"
#include "dfmessages/TriggerRecord_serialization.hpp"
#include "hdf5libs/HDF5RawDataFile.hpp"
#include "hdf5libs/hdf5filelayout/Nljs.hpp"
#include "hdf5libs/hdf5filelayout/Structs.hpp"
#include "hdf5libs/hdf5rawdatafile/Nljs.hpp"
#include "hdf5libs/hdf5rawdatafile/Structs.hpp"
#include "iomanager/IOManager.hpp"
#include "logging/Logging.hpp"
#include "serialization/Serialization.hpp"

using namespace dunedaq::iomanager;
using namespace dunedaq::hdf5libs;
using dataobj_t = nlohmann::json;

namespace dunedaq {
namespace datafilter {

using trigger_record_ptr_t =
    std::unique_ptr<dunedaq::daqdataformats::TriggerRecord>;

struct FilterResultWriterConfig {
    bool use_connectivity_service = false;
    int port = 15501;
    std::string server = "127.0.0.1";
    std::string server_trdispatcher = "127.0.0.1";

    std::string info_file_base = "FilterResultWriter";
    std::string odir = "/opt/tmp/chen";
    std::string output_h5_filename = "/opt/tmp/chen/h5_test.hdf5";
    std::string session_name = "FilterResultWriter test run";
    std::string ofile_pathname{};
    size_t num_apps = 1;
    size_t num_connections_per_group = 1;
    size_t num_groups = 1;
    size_t num_messages = 1;
    size_t message_size_kb = 1024;
    size_t num_runs = 1;
    size_t my_id = 1;
    size_t send_interval_ms = 100;
    int publish_interval = 10000;
    bool next_tr = false;

    size_t seq_number;
    size_t trigger_number;
    size_t trigger_timestamp;
    size_t run_number;
    size_t element_id;
    size_t detector_id;
    size_t error_bits;

    void make_init(const dataobj_t& jdata) {
        auto ini = jdata.get<dunedaq::dfbackend::filterresultwriter::Init>();
        dunedaq::dfbackend::filterresultwriter::ModInit data;
    }

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
            std::string conn_addr = "tcp://127." + std::to_string(third_byte) +
                                    "." + std::to_string(second_byte) + "." +
                                    std::to_string(first_byte) + ":15501";
        } else {
            conn_addr = "tcp://" + server + ":15501";
        }

        return conn_addr;
    }

    std::string get_pub_init_name() { return get_pub_init_name(my_id); }
    std::string get_pub_init_name(size_t id) {
        return "conn_init_" + std::to_string(id);
    }

    void configure_iomanager() {
        setenv("DUNEDAQ_PARTITION", session_name.c_str(), 0);

        Queues_t queues;
        Connections_t connections;

        //        for (size_t group = 0; group < num_groups; ++group) {
        //            for (size_t conn = 0; conn < num_connections_per_group;
        //            ++conn) {
        //                auto conn_addr = get_connection_ip(my_id, group,
        //                conn); TLOG() << "Adding connection with id "
        //                       << get_connection_name(my_id, group, conn)
        //                       << " and address " << conn_addr;
        //
        //                connections.emplace_back(Connection{
        //                    ConnectionId{get_connection_name(my_id, group,
        //                    conn),
        //                                 "TriggerRecord"},
        //                    conn_addr, ConnectionType::kPubSub});
        //            }
        //        }

        auto conn_addr = "tcp://" + server + ":" + std::to_string(port);
        connections.emplace_back(
            Connection{ConnectionId{"conn_A1_G0_C0_", "TriggerRecord"},
                       // conn_addr, ConnectionType::kPubSub});
                       conn_addr, ConnectionType::kSendRecv});

        //      for (size_t sub = 0; sub < num_apps; ++sub) {
        for (size_t sub = 0; sub < 3; ++sub) {
            auto port = 13000 + sub;
            std::string conn_addr =
                "tcp://" + server + ":" + std::to_string(port);
            TLOG() << "Adding control connection "
                   << "TR_tracking" + std::to_string(sub) << " with address "
                   << conn_addr;

            connections.emplace_back(Connection{
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

        //      for (size_t sub = 0; sub < num_apps; ++sub) {
        for (size_t sub = 0; sub < 3; ++sub) {
            auto port = 33000 + sub;
            std::string conn_addr =
                "tcp://" + server + ":" + std::to_string(port);
            TLOG() << "Adding control connection "
                   << "trwriter" + std::to_string(sub) << " with address "
                   << conn_addr;

            connections.emplace_back(Connection{
                ConnectionId{"trwriter" + std::to_string(sub), "init_t"},
                conn_addr, ConnectionType::kSendRecv});
        }

        // Create BookKeeping socket
        auto port = 83000;
        auto sub = 0;
        std::string conn_addrbookkeeping =
            "tcp://" + server + ":" + std::to_string(port);
        TLOG() << "Adding control connection "
               << "bookkeeping" + std::to_string(sub) << " with address "
               << conn_addrbookkeeping;

        connections.emplace_back(Connection{
            ConnectionId{"bookkeeping" + std::to_string(sub), "bk_t"},
            conn_addrbookkeeping, ConnectionType::kSendRecv});

        IOManager::get()->configure(
            queues, connections, use_connectivity_service,
            std::chrono::milliseconds(publish_interval));
    }
};

struct FilterResultWriter {
    FilterResultWriter() { TLOG() << "from FilterResultWriter"; }
    ~FilterResultWriter() {
        TLOG() << "from ~FilterResultWriter()";
        dunedaq::iomanager::IOManager::get()->reset();
    }

    explicit FilterResultWriter(FilterResultWriterConfig c) : config(c) {
        TLOG() << "from  FilterResultWriter";
        config.configure_iomanager();
    }
    FilterResultWriter(FilterResultWriter const&) = default;
    FilterResultWriter(FilterResultWriter&&) = default;
    FilterResultWriter& operator=(FilterResultWriter const&) = default;
    FilterResultWriter& operator=(FilterResultWriter&&) = default;

    struct FilterResultWriterInfo {
        size_t conn_id;
        size_t group_id;
        size_t messages_sent{0};
        size_t trigger_number;
        size_t trigger_timestamp;
        size_t run_number;
        size_t element_id;
        size_t detector_id;
        size_t error_bits;
        size_t fragment_type;
        std::string path_header;
        int n_frames;

        std::shared_ptr<SenderConcept<dunedaq::datafilter::Data>> sender;
        std::unique_ptr<std::thread> send_thread;
        std::chrono::milliseconds get_sender_time;

        FilterResultWriterInfo(size_t group, size_t conn)
            : conn_id(conn), group_id(group) {}
    };
    struct SubscriberInfo {
        size_t group_id;
        size_t conn_id;
        bool is_group_subscriber;
        std::unordered_map<size_t, size_t> last_sequence_received{0};
        std::atomic<size_t> msgs_received{0};
        std::atomic<size_t> msgs_with_error{0};
        std::chrono::milliseconds get_receiver_time;
        std::chrono::milliseconds add_callback_time;
        std::atomic<bool> complete{false};

        SubscriberInfo(size_t group, size_t conn)
            : group_id(group), conn_id(conn), is_group_subscriber(false) {}
        SubscriberInfo(size_t group)
            : group_id(group), conn_id(0), is_group_subscriber(true) {}

        std::string get_connection_name(FilterResultWriterConfig& config) {
            if (is_group_subscriber) {
                return config.get_group_connection_name(config.my_id, group_id);
            }
            return config.get_connection_name(config.my_id, group_id, conn_id);
        }
    };

    std::vector<std::shared_ptr<SubscriberInfo>> subscribers;
    FilterResultWriterConfig config;

    uint16_t data3[200000000];
    uint32_t nchannels = 64;
    uint32_t nsamples = 64;

    std::string path_header1;

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
                "trwriter0");
        auto init_receiver1 =
            dunedaq::get_iom_receiver<dunedaq::datafilter::Handshake>(
                "trwriter1");

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

    dunedaq::hdf5libs::hdf5filelayout::FileLayoutParams
    create_file_layout_params() {
        dunedaq::hdf5libs::hdf5filelayout::PathParams params_tpc;
        params_tpc.detector_group_type = "Detector_Readout";
        params_tpc.detector_group_name = "TPC";
        params_tpc.element_name_prefix = "Link";
        params_tpc.digits_for_element_number = 5;

        dunedaq::hdf5libs::hdf5filelayout::PathParamList param_list;
        param_list.push_back(params_tpc);

        dunedaq::hdf5libs::hdf5filelayout::FileLayoutParams layout_params;
        layout_params.path_param_list = param_list;
        layout_params.record_name_prefix = "TriggerRecord";
        layout_params.digits_for_record_number = 6;
        layout_params.digits_for_sequence_number = 0;
        layout_params.record_header_dataset_name = "TriggerRecordHeader";

        return layout_params;
    }

    dunedaq::hdf5libs::hdf5rawdatafile::SrcIDGeoIDMap create_srcid_geoid_map() {
        using nlohmann::json;

        dunedaq::hdf5libs::hdf5rawdatafile::SrcIDGeoIDMap map;
        json srcid_geoid_map = json::parse(R"(
        [
        {
          "source_id": 0,
          "geo_id": {
            "det_id": 3,
            "crate_id": 1,
            "slot_id": 0,
            "stream_id": 0
          }
        },
        {
          "source_id": 1,
          "geo_id": {
            "det_id": 3,
            "crate_id": 1,
            "slot_id": 0,
            "stream_id": 1
          }
        },
        {
          "source_id": 3,
          "geo_id": {
            "det_id": 3,
            "crate_id": 1,
            "slot_id": 1,
            "stream_id": 0
          }
        },
        {
          "source_id": 4,
          "geo_id": {
            "det_id": 3,
            "crate_id": 1,
            "slot_id": 1,
            "stream_id": 1
          }
        },
        {
          "source_id": 4,
          "geo_id": {
            "det_id": 2,
            "crate_id": 1,
            "slot_id": 0,
            "stream_id": 0
          }
        },
        {
          "source_id": 5,
          "geo_id": {
            "det_id": 2,
            "crate_id": 1,
            "slot_id": 0,
            "stream_id": 1
          }
        },
        {
          "source_id": 6,
          "geo_id": {
            "det_id": 2,
            "crate_id": 1,
            "slot_id": 1,
            "stream_id": 0
          }
        },
        {
          "source_id": 7,
          "geo_id": {
            "det_id": 2,
            "crate_id": 1,
            "slot_id": 1,
            "stream_id": 1
          }
        }
      ]
      )");

        return srcid_geoid_map.get<hdf5rawdatafile::SrcIDGeoIDMap>();
    }

    std::string time_point_to_string(
        const std::chrono::system_clock::time_point& tp) {
        // Convert the time_point to a time_t, which represents the time in
        // seconds since the epoch
        std::time_t time = std::chrono::system_clock::to_time_t(tp);

        // Convert the time_t to a tm structure for local time
        std::tm tm = *std::localtime(&time);

        auto since_epoch = tp.time_since_epoch();
        auto seconds =
            std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
        auto subseconds = since_epoch - seconds;
        auto nanoseconds =
            std::chrono::duration_cast<std::chrono::nanoseconds>(subseconds);

        // Use a stringstream to format the time as a string
        std::ostringstream oss;
        // oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "."
            << std::setfill('0') << std::setw(9) << nanoseconds.count();

        // Return the formatted string
        return oss.str();
    }

    void receive_tr(size_t run_number1) {
        bool handshake_done = false;
        std::atomic<unsigned int> received_cnt = 0;

        auto cb_receiver =
            dunedaq::get_iom_receiver<dunedaq::datafilter::Handshake>(
                "trwriter0");
        std::function<void(dunedaq::datafilter::Handshake)> str_receiver_cb =
            [&](dunedaq::datafilter::Handshake msg) {
                if (msg.msg_id == "write_tr") {
                    ++received_cnt;
                }
                TLOG_DEBUG(5) << "FilterResultWriter: TR receiver callback: "
                              << msg.msg_id;
            };

        cb_receiver->add_callback(str_receiver_cb);
        while (!handshake_done) {
            if (received_cnt == 1) handshake_done = true;
        }

        //        if (config.next_tr) {
        //            auto next_tr_sender =
        //                dunedaq::get_iom_sender<dunedaq::datafilter::Handshake>(
        //                    "twriter0");
        //            TLOG() << "send next_tr instruction";
        //            dunedaq::datafilter::Handshake q("next_tr");
        //            next_tr_sender->send(std::move(q), Sender::s_block);
        //        }

        TLOG_DEBUG(5) << "Setting up TRWriterInfo objects";
        for (size_t group = 0; group < config.num_groups; ++group) {
            // trwriters.push_back(std::make_shared<TRWriterInfo>(group));
            for (size_t conn = 0; conn < config.num_connections_per_group;
                 ++conn) {
                subscribers.push_back(
                    std::make_shared<SubscriberInfo>(group, conn));
            }
        }

        auto t1 = std::chrono::system_clock::now();
        dunedaq::datafilter::BookKeeping bk_info("bookkeeping0");
        bk_info.entry_id = time_point_to_string(t1);
        bk_info.conn_id = config.get_connection_name(config.my_id, 0, 0);
        bk_info.from_id = "FilterResultWriter";

        //        bk_info['entry_id'] = time_point_to_string(t1);
        //        bk_info['conn_id'] = config.get_connection_name(config.my_id,
        //        0, 0); bk_info['from_id'] = "FilterResultWriter";

        // the layout can be obtained from the tranfered TR.
        dunedaq::hdf5libs::hdf5filelayout::data_t flp_json_in;
        dunedaq::hdf5libs::hdf5filelayout::to_json(flp_json_in,
                                                   create_file_layout_params());

        // create src-geo id map; this should be replaced the correct src-geo
        // map from the transfered TR.
        auto srcid_geoid_map = create_srcid_geoid_map();

        std::atomic<std::chrono::steady_clock::time_point> last_received =
            std::chrono::steady_clock::now();
        TLOG_DEBUG(5) << "Adding callbacks for each subscriber";
        std::for_each(
            std::execution::par_unseq, std::begin(subscribers),
            std::end(subscribers),
            [=, &last_received](std::shared_ptr<SubscriberInfo> info) {
                auto recv_proc = [=, &last_received](trigger_record_ptr_t& tr) {
                    config.trigger_timestamp =
                        tr->get_fragments_ref().at(0)->get_trigger_timestamp();
                    config.trigger_number =
                        tr->get_fragments_ref().at(0)->get_trigger_number();
                    config.run_number =
                        tr->get_fragments_ref().at(0)->get_run_number();
                    int file_index = 0;

                    TLOG() << "run_number " << config.run_number
                           << ", trigger number: " << config.trigger_number;
                    info->msgs_received++;
                    last_received = std::chrono::steady_clock::now();

                    if (info->msgs_received == config.num_messages) {
                        TLOG() << "Complete condition reached, sending "
                                  "init message for "
                               << info->get_connection_name(config);
                        std::string app_name = "test";
                        config.ofile_pathname =
                            config.odir + "/" + config.output_h5_filename +
                            "_" + std::to_string(config.run_number) + "_" +
                            std::to_string(config.trigger_number) + ".hdf5";
                        TLOG() << "Writing the TR to " << config.ofile_pathname;

                        // create the file
                        std::unique_ptr<HDF5RawDataFile> h5file_ptr(
                            new HDF5RawDataFile(config.ofile_pathname,
                                                config.run_number, file_index,
                                                app_name, flp_json_in,
                                                srcid_geoid_map, ".writing",
                                                HighFive::File::Overwrite));

                        h5file_ptr->write(*tr);
                        h5file_ptr.reset();
                        info->complete = true;
                    }
                };

                auto before_receiver = std::chrono::steady_clock::now();
                auto receiver = dunedaq::get_iom_receiver<
                    std::unique_ptr<dunedaq::daqdataformats::TriggerRecord>>(
                    info->get_connection_name(config));
                auto after_receiver = std::chrono::steady_clock::now();
                receiver->add_callback(recv_proc);
                auto after_callback = std::chrono::steady_clock::now();
                info->get_receiver_time =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        after_receiver - before_receiver);
                info->add_callback_time =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        after_callback - after_receiver);
            });

        // ACK to datafilter that we succefully received the TR
        TLOG() << "Send bookkeeping info to datafilter server";
        bk_info.tr_status = "received";
        bk_info.tr_header_info.push_back(
            {"run_number", std::to_string(config.run_number)});
        bk_info.tr_header_info.push_back(
            {"trigger_number", std::to_string(config.trigger_number)});
        bk_info.tr_header_info.push_back(
            {"tr_writer_pathname", config.ofile_pathname});

        auto init_bookkeeping_sender =
            dunedaq::get_iom_sender<dunedaq::datafilter::BookKeeping>(
                "bookkeeping0");
        init_bookkeeping_sender->send(std::move(bk_info), Sender::s_block);

        //        if (config.next_tr) {
        //            auto next_tr_sender =
        //                dunedaq::get_iom_sender<dunedaq::datafilter::Handshake>(
        //                    "trwriter0");
        //            TLOG() << "send wait for next instruction";
        //            dunedaq::datafilter::Handshake q("wait");
        //            next_tr_sender->send(std::move(q), Sender::s_block);
        //        }

        TLOG_DEBUG(5) << "Starting wait loop for receives to complete";
        bool all_done = false;
        while (!all_done) {
            size_t recvrs_done = 0;
            for (auto& sub : subscribers) {
                if (sub->complete.load()) recvrs_done++;
            }
            TLOG_DEBUG(6) << "Done: " << recvrs_done << ", expected: "
                          << config.num_groups *
                                 config.num_connections_per_group;
            all_done = recvrs_done >=
                       config.num_groups * config.num_connections_per_group;
            if (!all_done) std::this_thread::sleep_for(1ms);
        }
        TLOG_DEBUG(5) << "Removing callbacks";
        for (auto& info : subscribers) {
            auto receiver = dunedaq::get_iom_receiver<trigger_record_ptr_t>(
                info->get_connection_name(config));
            receiver->remove_callback();
        }

        auto cb_receiver1 =
            dunedaq::get_iom_receiver<dunedaq::datafilter::Handshake>(
                "trwriter0");
        cb_receiver1->remove_callback();

        subscribers.clear();
        TLOG_DEBUG(5) << "receive() done";
    }

    void send_next_tr(size_t run_number, pid_t subscriber_pid) {
        bool handshake_done = false;

        std::atomic<unsigned int> sent_cnt = 0;

        auto sender_next_tr =
            dunedaq::get_iom_sender<dunedaq::datafilter::Handshake>(
                "trdispatcher1");

        // std::chrono::milliseconds timeout(100);
        dunedaq::datafilter::Handshake sent_t1("trdispatcher1");
        // sender_next_tr->send(std::move(sent_t1), timeout);
        sender_next_tr->send(std::move(sent_t1), Sender::s_block);
    }
};

}  // namespace datafilter
DUNE_DAQ_SERIALIZABLE(dunedaq::datafilter::Handshake, "init_t");
DUNE_DAQ_SERIALIZABLE(dunedaq::datafilter::BookKeeping, "bk_t");
// DUNE_DAQ_SERIALIZABLE(dunedaq::datafilter::BookKeeping_json, "bk_t");
}  // namespace dunedaq

#endif  // DFBACKEND_INCLUDE_FILTERRESULTWRITER_HPP_
