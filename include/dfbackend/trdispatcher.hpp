#ifndef DFBACKEND_INCLUDE_TRDISPATCHER_HPP_
#define DFBACKEND_INCLUDE_TRDISPATCHER_HPP_

#include <sys/wait.h>

#include <algorithm>
#include <execution>
#include <fstream>

#include "datafilter/data_struct.hpp"
#include "detdataformats/DetID.hpp"
// #include "dfbackend/HDF5FromStorage.hpp"
//  #include "fddetdataformats/WIBEthFrame.hpp"
#include "dfmessages/TriggerRecord_serialization.hpp"
#include "hdf5libs/HDF5RawDataFile.hpp"
#include "hdf5libs/hdf5filelayout/Nljs.hpp"
#include "hdf5libs/hdf5rawdatafile/Nljs.hpp"
#include "iomanager/IOManager.hpp"
#include "logging/Logging.hpp"

using namespace dunedaq::hdf5libs;
using namespace dunedaq::daqdataformats;
using namespace dunedaq::detdataformats;
using namespace dunedaq::iomanager;

namespace dunedaq {
namespace datafilter {

using trigger_record_ptr_t =
    std::unique_ptr<dunedaq::daqdataformats::TriggerRecord>;

struct TRDispatcherConfig {
    bool use_connectivity_service = false;  // unsed for now
    int port = 15500;
    std::string server = "127.0.0.1";
    std::string server_trdispatcher = "127.0.0.1";

    std::string info_file_base = "trdispatcher";
    std::string session_name = "trdispatcher test run";
    size_t num_apps = 1;
    size_t num_connections_per_group = 1;
    size_t num_groups = 1;
    size_t num_messages = 1;
    size_t message_size_kb = 1024;
    size_t num_runs = 2;
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

    std::string input_h5_filename =
        "/lcg/storage19/test-area/dune/trigger_records/"
        "swtest_run001039_0000_dataflow0_datawriter_0_20231103T121050.hdf5";
    std::string output_h5_filename = "/opt/tmp/chen/h5_test.hdf5";
    size_t write_fragment_type = 1;  // 0 -> TPC; 1 -> Trigger

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
        int third_byte = app_id + 1;     // 1-254
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

        auto conn_addr = "tcp://" + server + ":" + std::to_string(port);
        connections.emplace_back(
            Connection{ConnectionId{"conn_A0_G0_C0_", "TriggerRecord"},
                       // conn_addr, ConnectionType::kPubSub});
                       conn_addr, ConnectionType::kSendRecv});

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
        //                                 // "data_t"},
        //                                 "TriggerRecord"},
        //                    conn_addr, ConnectionType::kPubSub});
        //            }
        //        }

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

        // for (size_t sub = 0; sub < num_apps; ++sub) {
        for (size_t sub = 0; sub < 3; ++sub) {
            auto port = 23000 + sub;
            std::string conn_addr =
                "tcp://" + server_trdispatcher + ":" + std::to_string(port);
            TLOG() << "Adding control connection "
                   << "trdispatcher" + std::to_string(sub) << " with address "
                   << conn_addr;

            connections.emplace_back(Connection{
                ConnectionId{"trdispatcher" + std::to_string(sub), "init_t"},
                conn_addr, ConnectionType::kSendRecv});
        }

        IOManager::get()->configure(queues, connections,
                                    use_connectivity_service,
                                    std::chrono::milliseconds(1000ms));
    }
};

// struct TRDispatcher {
//     // TRDispatcher();
//     const std::string storage_pathname =
//         "/lcg/storage19/test-area/dune-v4-spack-integration2/sourcecode/"
//         "daqconf/config/";
//     const std::string json_file = "hdf5_files_list.json";
//
//     //    void init() {
//     //        dunedaq::trdispatcher::HDF5FromStorage s(storage_pathname,
//     //        json_file); s.print();
//     //
//     //        for (auto file : s.hdf5_files_to_transfer) {
//     //            std::cout << "main: files to transfer" << file << "\n";
//     //        }
//     //    }
// };

struct TRDispatcher {
    struct TRDispatcherInfo {
        size_t conn_id;
        size_t group_id;
        size_t messages_sent{0};
        size_t trigger_number;
        size_t trigger_timestamp;
        size_t run_number = 53;
        size_t element_id;
        size_t detector_id;
        size_t error_bits;
        // dunedaq::daqdataformats::Fragment fragment_type;
        size_t fragment_type;
        std::string path_header;
        int n_frames;

        std::shared_ptr<SenderConcept<trigger_record_ptr_t>> sender;
        std::unique_ptr<std::thread> send_thread;
        std::chrono::milliseconds get_sender_time;

        TRDispatcherInfo(size_t group, size_t conn)
            : conn_id(conn), group_id(group) {}
    };

    std::vector<std::shared_ptr<TRDispatcherInfo>> trdispatchers;
    dunedaq::datafilter::TRDispatcherConfig config;

    size_t run_number = 53;
    size_t fragment_size = 100;
    size_t element_count_tpc = 4;
    size_t element_count_pds = 4;
    size_t element_count_ta = 4;
    size_t element_count_tc = 1;
    const size_t components_per_record = element_count_tpc + element_count_pds +
                                         element_count_ta + element_count_tc;

    uint16_t data3[200000000];
    uint32_t nchannels = 64;
    uint32_t nsamples = 64;

    std::string path_header1;

    std::string input_h5_filename =
        "/lcg/storage19/test-area/dune/trigger_records/"
        "swtest_run001039_0000_dataflow0_datawriter_0_20231103T121050.hdf5";
    // HDF5RawDataFile h5_file(config.input_h5_filename);
    // HDF5RawDataFile h5_file1(input_h5_filename);

    ConnectionId conn_id;
    ConnectionId conn_id1;

    TRDispatcher(TRDispatcherConfig c) : config(c) {
        TLOG() << "from trdispatcher()";
        config.configure_iomanager();
    }
    ~TRDispatcher() { dunedaq::iomanager::IOManager::get()->reset(); }
    TRDispatcher(TRDispatcher const&) = default;
    TRDispatcher(TRDispatcher&&) = default;
    TRDispatcher& operator=(TRDispatcher const&) = default;
    TRDispatcher& operator=(TRDispatcher&&) = default;

    // void open() { HDF5RawDataFile h5_file(config.input_h5_filename); }
    void init(size_t run_number) {
        TLOG() << "Getting init sender";
        auto init_sender =
            dunedaq::get_iom_sender<dunedaq::datafilter::Handshake>(
                //                "trdispatcher0");
                "TR_tracking0");
        auto init_receiver =
            dunedaq::get_iom_receiver<dunedaq::datafilter::Handshake>(
                //                "trdispatcher1");
                "TR_tracking2");

        std::atomic<std::chrono::steady_clock::time_point> last_received =
            std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - last_received.load())
                   .count() < 50000) {
            // Handshake q(config.my_id, -1, 0, run_number);
            dunedaq::datafilter::Handshake q("start");
            init_sender->send(std::move(q), Sender::s_block);
            dunedaq::datafilter::Handshake recv;
            recv = init_receiver->receive(std::chrono::milliseconds(100));
            std::this_thread::sleep_for(100ms);
            if (recv.msg_id == "gotit") TLOG() << "Receiver got it";
            break;
        }
        TLOG() << "End init()";
    }

    // generate a dummy test trigger record to be send to datafilter
    dunedaq::datafilter::trigger_record_ptr_t create_trigger_record(
        uint64_t trig_num) {
        // test setup our dummy_data
        std::vector<char> dummy_vector(fragment_size);
        char* dummy_data = dummy_vector.data();

        // get a timestamp for this trigger
        int64_t ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                         system_clock::now().time_since_epoch())
                         .count();

        // create TriggerRecordHeader
        dunedaq::daqdataformats::TriggerRecordHeaderData trh_data;
        trh_data.trigger_number = trig_num;
        trh_data.trigger_timestamp = ts;
        trh_data.num_requested_components = components_per_record;
        trh_data.run_number = run_number;
        trh_data.sequence_number = 0;
        trh_data.max_sequence_number = 1;
        trh_data.element_id = dunedaq::daqdataformats::SourceID(
            dunedaq::daqdataformats::SourceID::Subsystem::kTRBuilder, 0);

        dunedaq::daqdataformats::TriggerRecordHeader trh(&trh_data);

        // create our TriggerRecord
        auto tr = std::make_unique<dunedaq::daqdataformats::TriggerRecord>(trh);

        // loop over elements tpc
        for (size_t ele_num = 0; ele_num < element_count_tpc; ++ele_num) {
            // create our fragment
            dunedaq::daqdataformats::FragmentHeader fh;
            fh.trigger_number = trig_num;
            fh.trigger_timestamp = ts;
            fh.window_begin = ts;
            fh.window_end = ts;
            fh.run_number = run_number;
            fh.fragment_type =
                static_cast<dunedaq::daqdataformats::fragment_type_t>(
                    dunedaq::daqdataformats::FragmentType::kWIB);
            fh.sequence_number = 0;
            fh.detector_id = static_cast<uint16_t>(
                dunedaq::detdataformats::DetID::Subdetector::kHD_TPC);
            fh.element_id = dunedaq::daqdataformats::SourceID(
                dunedaq::daqdataformats::SourceID::Subsystem::kDetectorReadout,
                ele_num);

            std::unique_ptr<dunedaq::daqdataformats::Fragment> frag_ptr(
                new dunedaq::daqdataformats::Fragment(dummy_data,
                                                      fragment_size));
            frag_ptr->set_header_fields(fh);

            // add fragment to TriggerRecord
            tr->add_fragment(std::move(frag_ptr));

        }  // end loop over elements

        // loop over elements pds
        for (size_t ele_num = 0; ele_num < element_count_pds; ++ele_num) {
            // create our fragment
            dunedaq::daqdataformats::FragmentHeader fh;
            fh.trigger_number = trig_num;
            fh.trigger_timestamp = ts;
            fh.window_begin = ts;
            fh.window_end = ts;
            fh.run_number = run_number;
            fh.fragment_type =
                static_cast<dunedaq::daqdataformats::fragment_type_t>(
                    dunedaq::daqdataformats::FragmentType::kDAPHNE);
            fh.sequence_number = 0;
            fh.detector_id = static_cast<uint16_t>(
                dunedaq::detdataformats::DetID::Subdetector::kHD_PDS);
            fh.element_id = dunedaq::daqdataformats::SourceID(
                dunedaq::daqdataformats::SourceID::Subsystem::kDetectorReadout,
                ele_num + element_count_tpc);

            std::unique_ptr<dunedaq::daqdataformats::Fragment> frag_ptr(
                new dunedaq::daqdataformats::Fragment(dummy_data,
                                                      fragment_size));
            frag_ptr->set_header_fields(fh);

            // add fragment to TriggerRecord
            // tr.add_fragment(std::move(frag_ptr));
            tr->add_fragment(std::move(frag_ptr));

        }  // end loop over elements

        // loop over TriggerActivity
        for (size_t ele_num = 0; ele_num < element_count_ta; ++ele_num) {
            // create our fragment
            dunedaq::daqdataformats::FragmentHeader fh;
            fh.trigger_number = trig_num;
            fh.trigger_timestamp = ts;
            fh.window_begin = ts;
            fh.window_end = ts;
            fh.run_number = run_number;
            fh.fragment_type =
                static_cast<dunedaq::daqdataformats::fragment_type_t>(
                    dunedaq::daqdataformats::FragmentType::kTriggerActivity);
            fh.sequence_number = 0;
            fh.detector_id = static_cast<uint16_t>(
                dunedaq::detdataformats::DetID::Subdetector::kDAQ);
            fh.element_id = dunedaq::daqdataformats::SourceID(
                dunedaq::daqdataformats::SourceID::Subsystem::kTrigger,
                ele_num);

            std::unique_ptr<dunedaq::daqdataformats::Fragment> frag_ptr(
                new dunedaq::daqdataformats::Fragment(dummy_data,
                                                      fragment_size));
            frag_ptr->set_header_fields(fh);

            // add fragment to TriggerRecord
            tr->add_fragment(std::move(frag_ptr));

        }  // end loop over elements

        // loop over TriggerCandidate
        for (size_t ele_num = 0; ele_num < element_count_tc; ++ele_num) {
            // create our fragment
            dunedaq::daqdataformats::FragmentHeader fh;
            fh.trigger_number = trig_num;
            fh.trigger_timestamp = ts;
            fh.window_begin = ts;
            fh.window_end = ts;
            fh.run_number = run_number;
            fh.fragment_type =
                static_cast<dunedaq::daqdataformats::fragment_type_t>(
                    dunedaq::daqdataformats::FragmentType::kTriggerCandidate);
            fh.sequence_number = 0;
            fh.detector_id = static_cast<uint16_t>(
                dunedaq::detdataformats::DetID::Subdetector::kDAQ);
            fh.element_id = dunedaq::daqdataformats::SourceID(
                dunedaq::daqdataformats::SourceID::Subsystem::kTrigger,
                ele_num + element_count_ta);

            std::unique_ptr<dunedaq::daqdataformats::Fragment> frag_ptr(
                new dunedaq::daqdataformats::Fragment(dummy_data,
                                                      fragment_size));
            frag_ptr->set_header_fields(fh);

            // add fragment to TriggerRecord
            tr->add_fragment(std::move(frag_ptr));

        }  // end loop over elements

        dunedaq::datafilter::trigger_record_ptr_t temp = std::move(tr);
        return temp;
    }

    // send trigger records from self generated TR
    void send_tr(size_t dataflow_run_number, pid_t subscriber_pid) {
        std::ostringstream ss;

        auto init_sender =
            dunedaq::get_iom_sender<dunedaq::datafilter::Handshake>(
                "TR_tracking2");

        dunedaq::datafilter::Handshake sent_t1("next_tr");
        init_sender->send(std::move(sent_t1), Sender::s_block);

        //        if (config.next_tr) {
        //            auto init_receiver =
        //                dunedaq::get_iom_receiver<dunedaq::datafilter::Handshake>(
        //                    "TR_tracking2");
        //        }
        std::unordered_map<int, std::set<size_t>> completed_receiver_tracking;
        std::mutex tracking_mutex;

        //        for (size_t group = 0; group < config.num_groups; ++group)
        //        {
        //            for (size_t conn = 0; conn <
        //            config.num_connections_per_group;
        //                 ++conn) {
        //                auto info =
        //                std::make_shared<TRDispatcherInfo>(group, conn);
        auto info = std::make_shared<TRDispatcherInfo>(0, 0);
        trdispatchers.push_back(info);
        //            }
        //        }

        TLOG_DEBUG(7) << "Getting publisher objects for each connection";
        std::for_each(
            std::execution::par_unseq, std::begin(trdispatchers),
            std::end(trdispatchers),
            [=](std::shared_ptr<TRDispatcherInfo> info) {
                auto before_sender = std::chrono::steady_clock::now();

                info->sender = dunedaq::get_iom_sender<trigger_record_ptr_t>(
                    config.get_connection_name(config.my_id, info->group_id,
                                               info->conn_id));
                auto after_sender = std::chrono::steady_clock::now();
                info->get_sender_time =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        after_sender - before_sender);
            });

        TLOG_DEBUG(7) << "Starting publish threads";
        std::for_each(
            std::execution::par_unseq, std::begin(trdispatchers),
            std::end(trdispatchers),
            [=, &completed_receiver_tracking,
             &tracking_mutex](std::shared_ptr<TRDispatcherInfo> info) {
                info->send_thread.reset(new std::thread(
                    [=, &completed_receiver_tracking, &tracking_mutex]() {
                        bool complete_received = false;

                        std::this_thread::sleep_for(100ms);
                        while (!complete_received) {
                            TLOG() << "Sender message: generate trigger "
                                      "record";
                            dunedaq::datafilter::trigger_record_ptr_t
                                temp_record(create_trigger_record(1));

                            TLOG() << "Start sending  trigger record";
                            info->sender->try_send(
                                std::move(temp_record),
                                std::chrono::milliseconds(
                                    config.send_interval_ms));
                            TLOG() << "End sending trigger record";
                            ++info->messages_sent;
                            {
                                std::lock_guard<std::mutex> lk(tracking_mutex);
                                if ((completed_receiver_tracking.count(
                                         info->group_id) &&
                                     completed_receiver_tracking[info->group_id]
                                         .count(info->conn_id)) ||
                                    completed_receiver_tracking.count(-1)) {
                                    TLOG() << "Complete_received";
                                    complete_received = true;
                                }
                            }
                            complete_received = true;
                            break;
                        }  // while loop
                    }));
            });

        TLOG_DEBUG(7) << "Joining send threads";
        for (auto& sender : trdispatchers) {
            sender->send_thread->join();
            sender->send_thread.reset(nullptr);
        }
    }

    // send trigger records from generated hdf5 files.
    void send_tr_from_hdf5file(size_t dataflow_run_number,
                               pid_t subscriber_pid) {
        std::ostringstream ss;

        auto init_sender =
            dunedaq::get_iom_sender<dunedaq::datafilter::Handshake>(
                "TR_tracking2");

        dunedaq::datafilter::Handshake sent_t1("next_tr");
        init_sender->send(std::move(sent_t1), Sender::s_block);

        //        if (config.next_tr) {
        //            auto init_receiver =
        //                dunedaq::get_iom_receiver<dunedaq::datafilter::Handshake>(
        //                    "TR_tracking2");
        //        }

        std::unordered_map<int, std::set<size_t>> completed_receiver_tracking;
        std::mutex tracking_mutex;

        for (size_t group = 0; group < config.num_groups; ++group) {
            for (size_t conn = 0; conn < config.num_connections_per_group;
                 ++conn) {
                auto info = std::make_shared<TRDispatcherInfo>(group, conn);
                // auto info = std::make_shared<TRDispatcherInfo>(0, 0);
                trdispatchers.push_back(info);
            }
        }

        TLOG_DEBUG(7) << "Getting publisher objects for each connection";
        std::for_each(
            std::execution::par_unseq, std::begin(trdispatchers),
            std::end(trdispatchers),
            [=](std::shared_ptr<TRDispatcherInfo> info) {
                auto before_sender = std::chrono::steady_clock::now();
                //                    info->sender =
                //                    dunedaq::get_iom_sender<dunedaq::datafilter::Data>(
                info->sender = dunedaq::get_iom_sender<trigger_record_ptr_t>(
                    config.get_connection_name(config.my_id, info->group_id,
                                               info->conn_id));
                auto after_sender = std::chrono::steady_clock::now();
                info->get_sender_time =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        after_sender - before_sender);
            });

        TLOG_DEBUG(7) << "Starting publish threads";
        std::for_each(
            std::execution::par_unseq, std::begin(trdispatchers),
            std::end(trdispatchers),
            [=, &completed_receiver_tracking,
             &tracking_mutex](std::shared_ptr<TRDispatcherInfo> info) {
                info->send_thread.reset(new std::thread(
                    [=, &completed_receiver_tracking, &tracking_mutex]() {
                        bool complete_received = false;

                        std::ostringstream ss;
                        std::this_thread::sleep_for(100ms);
                        while (!complete_received) {
                            TLOG() << "Sender message: trigger "
                                      "record";
                            std::string ifilename = config.input_h5_filename;
                            TLOG() << "Reading " << ifilename;

                            HDF5RawDataFile h5_file(ifilename);
                            auto records = h5_file.get_all_record_ids();
                            ss << "\nNumber of records: " << records.size();
                            if (records.empty()) {
                                ss << "\n\nNO TRIGGER RECORDS FOUND";
                                TLOG() << ss.str();
                                exit(0);
                            }
                            auto first_rec = *(records.begin());
                            auto last_rec = *(
                                std::next(records.begin(), records.size() - 1));

                            ss << "\n\tFirst trigger record: "
                               << first_rec.first << "," << first_rec.second;
                            ss << "\n\tLast trigger record: " << last_rec.first
                               << "," << last_rec.second;

                            TLOG() << ss.str();
                            ss.str("");

                            for (auto const& rid : records) {
                                auto record_header_dataset =
                                    h5_file.get_record_header_dataset_path(rid);
                                auto tr = h5_file.get_trigger_record(rid);

                                // SERIALIZE
                                auto bytes = dunedaq::serialization::serialize(
                                    tr, dunedaq::serialization::kMsgPack);
                                // DESERIALIZE
                                auto deserialized =
                                    dunedaq::serialization::deserialize<
                                        trigger_record_ptr_t>(bytes);

                                // dunedaq::datafilter::trigger_record_ptr_t
                                //     temp_record(&tr);

                                info->sender->try_send(
                                    std::move(deserialized),
                                    std::chrono::milliseconds(50));
                            }

                            ++info->messages_sent;
                            {
                                std::lock_guard<std::mutex> lk(tracking_mutex);
                                if ((completed_receiver_tracking.count(
                                         info->group_id) &&
                                     completed_receiver_tracking[info->group_id]
                                         .count(info->conn_id)) ||
                                    completed_receiver_tracking.count(-1)) {
                                    TLOG() << "Complete_received";
                                    complete_received = true;
                                }
                            }
                            complete_received = true;
                            break;
                        }  // while loop
                    }));
            });

        TLOG_DEBUG(7) << "Joining send threads";
        for (auto& sender : trdispatchers) {
            sender->send_thread->join();
            sender->send_thread.reset(nullptr);
        }
    }

    void receive(size_t dataflow_run_number1, pid_t subscriber_pid,
                 bool is_hdf5file) {
        bool handshake_done = false;
        std::atomic<unsigned int> received_cnt = 0;

        auto cb_receiver =
            dunedaq::get_iom_receiver<dunedaq::datafilter::Handshake>(
                "trdispatcher0");
        std::function<void(dunedaq::datafilter::Handshake)> str_receiver_cb =
            [&](dunedaq::datafilter::Handshake msg) {
                if (msg.msg_id == "trdispatcher0") {
                    ++received_cnt;
                }
                TLOG() << "Received next TR instruction from filter "
                          "orchestrator: "
                       << msg.msg_id;
            };

        cb_receiver->add_callback(str_receiver_cb);
        while (!handshake_done) {
            if (received_cnt == 1) handshake_done = true;
        }

        if (is_hdf5file) {
            send_tr_from_hdf5file(dataflow_run_number1, subscriber_pid);
        } else {
            send_tr(dataflow_run_number1, subscriber_pid);
        }
    }
};

}  // namespace datafilter
DUNE_DAQ_SERIALIZABLE(dunedaq::datafilter::Data, "data_t");
DUNE_DAQ_SERIALIZABLE(dunedaq::datafilter::Handshake, "init_t");
}  // namespace dunedaq

#endif  // DFBACKEND_INCLUDE_TRDISPATCHER_HPP_
