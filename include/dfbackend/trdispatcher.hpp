#include <sys/wait.h>

#include <algorithm>
#include <execution>
#include <fstream>

#include "datafilter/data_struct.hpp"
#include "detdataformats/DetID.hpp"
#include "dfbackend/HDF5FromStorage.hpp"
#include "fddetdataformats/WIBEthFrame.hpp"
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
namespace trdispatcher {

struct TRDispatcherConfig {
    bool use_connectivity_service = false;  // unsed for now
    int port = 5000;
    std::string server = "localhost";

    std::string info_file_base = "trdispatcher";
    std::string session_name = "iomanager : trdispatcher";
    size_t num_apps = 1;
    size_t num_connections_per_group = 1;
    size_t num_groups = 1;
    size_t num_messages = 1;
    size_t message_size_kb = 1024;
    size_t num_runs = 2;
    size_t my_id = 0;
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
    // size_t fragment_type;
    // dunedaq::daqdataformats::Fragment fragment_type;

    std::string input_h5_filename =
        "/lcg/storage19/test-area/dune/trigger_records/"
        "swtest_run001039_0000_dataflow0_datawriter_0_20231103T121050.hdf5";
    std::string output_h5_filename = "/opt/tmp/chen/h5_test.hdf5";
    size_t write_fragment_type = 1;  // 0 -> TPC; 1 -> Trigger
    // std::string hw_map_file_name="testwriter_hardwaremap.txt";

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

        std::string conn_addr = "tcp://127." + std::to_string(third_byte) +
                                "." + std::to_string(second_byte) + "." +
                                std::to_string(first_byte) + ":15500";

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

        for (size_t group = 0; group < num_groups; ++group) {
            for (size_t conn = 0; conn < num_connections_per_group; ++conn) {
                auto conn_addr = get_connection_ip(my_id, group, conn);
                TLOG() << "Adding connection with id "
                       << get_connection_name(my_id, group, conn)
                       << " and address " << conn_addr;

                connections.emplace_back(Connection{
                    ConnectionId{get_connection_name(my_id, group, conn),
                                 "data_t"},
                    conn_addr, ConnectionType::kPubSub});
            }
        }

        //      for (size_t sub = 0; sub < num_apps; ++sub) {
        for (size_t sub = 0; sub < 3; ++sub) {
            auto port = 13000 + sub;
            std::string conn_addr = "tcp://127.0.0.1:" + std::to_string(port);
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
            std::string conn_addr = "tcp://127.0.0.1:" + std::to_string(port);
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

struct TRDispatcher {
    // TRDispatcher();
    const std::string storage_pathname =
        "/lcg/storage19/test-area/dune-v4-spack-integration2/sourcecode/"
        "daqconf/config/";
    const std::string json_file = "hdf5_files_list.json";

    void init() {
        dunedaq::trdispatcher::HDF5FromStorage s(storage_pathname, json_file);
        s.print();

        for (auto file : s.hdf5_files_to_transfer) {
            std::cout << "main: files to transfer" << file << "\n";
        }
    }
};

struct PublisherTest {
    struct PublisherInfo {
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

        PublisherInfo(size_t group, size_t conn)
            : conn_id(conn), group_id(group) {}
    };

    std::vector<std::shared_ptr<PublisherInfo>> publishers;
    TRDispatcherConfig config;

    uint16_t data3[200000000];
    uint32_t nchannels = 64;
    uint32_t nsamples = 64;

    std::string path_header1;

    std::string input_h5_filename =
        "/lcg/storage19/test-area/dune/trigger_records/"
        "swtest_run001039_0000_dataflow0_datawriter_0_20231103T121050.hdf5";
    // HDF5RawDataFile h5_file(config.input_h5_filename);
    // HDF5RawDataFile h5_file1(input_h5_filename);

    explicit PublisherTest(TRDispatcherConfig c) : config(c) {}

    void open() { HDF5RawDataFile h5_file(config.input_h5_filename); }
    void init(size_t run_number) {
        TLOG() << "Getting init sender";
        // auto init_sender =
        // dunedaq::get_iom_sender<dunedaq::datafilter::Handshake>(config.get_pub_init_name());
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

    void send(size_t run_number, pid_t subscriber_pid) {
        std::ostringstream ss;

        auto init_receiver =
            dunedaq::get_iom_receiver<dunedaq::datafilter::Handshake>(
                "TR_tracking2");
        //"trdispatcher2");
        std::unordered_map<int, std::set<size_t>> completed_receiver_tracking;
        std::mutex tracking_mutex;

        //    for (size_t group = 0; group < config.num_groups; ++group) {
        //      for (size_t conn = 0; conn < config.num_connections_per_group;
        //      ++conn) {
        // auto info = std::make_shared<PublisherInfo>(group, conn);
        auto info = std::make_shared<PublisherInfo>(0, 0);
        publishers.push_back(info);
        //      }
        //    }

        TLOG() << "Getting publisher objects for each connection";
        std::for_each(
            std::execution::par_unseq, std::begin(publishers),
            std::end(publishers), [=](std::shared_ptr<PublisherInfo> info) {
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

        auto size = 1024;
        // sending random vector
        //    std::vector<int> send_values;
        ////    std::generate(send_values.begin(),send_values.end(), []() {
        ////            return rand();
        ////            });
        //
        //    for (auto j = 0; j < size*1024; j++)
        //    {
        //
        //        send_values.push_back(rand());
        //    }
        //

        TLOG() << "Starting publish threads";
        std::for_each(
            std::execution::par_unseq, std::begin(publishers),
            std::end(publishers),
            [=, &completed_receiver_tracking,
             &tracking_mutex](std::shared_ptr<PublisherInfo> info) {
                info->send_thread.reset(new std::thread(
                    [=, &completed_receiver_tracking, &tracking_mutex]() {
                        bool complete_received = false;

                        HDF5RawDataFile h5_file(config.input_h5_filename);
                        auto run_number =
                            h5_file.get_attribute<unsigned int>("run_number");
                        auto file_index =
                            h5_file.get_attribute<unsigned int>("file_index");
                        auto creation_timestamp =
                            h5_file.get_attribute<std::string>(
                                "creation_timestamp");
                        auto app_name = h5_file.get_attribute<std::string>(
                            "application_name");

                        auto records = h5_file.get_all_record_ids();

                        while (!complete_received) {
                            for (auto const& rid : records) {
                                auto record_header_dataset =
                                    h5_file.get_record_header_dataset_path(rid);
                                auto tr = h5_file.get_trigger_record(rid);
                                auto trh_ptr = h5_file.get_trh_ptr(rid);
                                auto trig_num = trh_ptr->get_trigger_number();
                                auto num_requested_components =
                                    trh_ptr->get_num_requested_components();

                                auto seq_num = trh_ptr->get_sequence_number();
                                auto max_seq_num =
                                    trh_ptr->get_max_sequence_number();

                                path_header1 = record_header_dataset;
                                config.seq_number = seq_num;
                                config.trigger_number = trig_num;

                                auto frag_paths =
                                    h5_file.get_fragment_dataset_paths(rid);
                                auto all_frag_paths =
                                    h5_file.get_all_fragment_dataset_paths();
                                for (auto const& path : frag_paths) {
                                    auto frag_ptr = h5_file.get_frag_ptr(path);
                                    auto fragment_size = frag_ptr->get_size();
                                    auto fragment_type =
                                        frag_ptr->get_fragment_type();

                                    auto elem_id = frag_ptr->get_element_id();

                                    // config.element_id=elem_id;
                                    auto data = frag_ptr->get_data();

                                    // std::vector<uint64_t*>& data1 =
                                    // *reinterpret_cast<std::vector<uint64_t*>
                                    // *>(data);

                                    int nframes =
                                        (fragment_size -
                                         sizeof(
                                             daqdataformats::FragmentHeader)) /
                                        sizeof(fddetdataformats::WIBEthFrame);
                                    // std::cout<<"fragment_size"<<fragment_size<<"
                                    // "<<nframes<<"\n";
                                    for (auto i = 0; i < nframes; ++i) {
                                        auto fr = reinterpret_cast<
                                            fddetdataformats::WIBEthFrame*>(
                                            static_cast<char*>(data) +
                                            i * sizeof(fddetdataformats::
                                                           WIBEthFrame));
                                        for (auto j = 0; j < nsamples; ++j) {
                                            for (auto k = 0; k < nchannels;
                                                 ++k) {
                                                data3[(nsamples * nchannels) *
                                                          i +
                                                      nchannels * j + k] =
                                                    fr->get_adc(k, j);
                                                // std::cout<<"=======================>"<<i<<"
                                                // "<<j<<"
                                                // "<<data3[nsamples*nchannels*i
                                                // + nchannels*j+k]<<'\n';
                                            }
                                        }
                                    }
                                    //        }
                                    //    }

                                    //          while (!complete_received) {
                                    TLOG()
                                        << "Sending message "
                                        << info->messages_sent << " with size "
                                        << config.message_size_kb * 1024
                                        << " bytes to connection "
                                        << config.get_connection_name(
                                               config.my_id, info->group_id,
                                               info->conn_id);

                                    info->trigger_number =
                                        config.trigger_number;
                                    info->run_number = run_number;
                                    info->path_header = path;
                                    info->n_frames = nframes;
                                    //            info->element_id=elem_id;
                                    //            info->detector_id=detector_id;
                                    //            info->error_bits=error_bits;
                                    //            info->fragment_type=fragment_type;

                                    dunedaq::datafilter::Data d(
                                        info->messages_sent,
                                        info->trigger_number,
                                        info->trigger_timestamp,
                                        info->run_number, info->element_id,
                                        info->detector_id, info->error_bits,
                                        info->fragment_type, info->path_header,
                                        info->n_frames, config.my_id,
                                        info->group_id, info->conn_id,
                                        config.message_size_kb * 1024);
                                    auto v1 = 999;
                                    for (auto j = 0; j < size * 1024; j++) {
                                        // v1=rand();
                                        // d.contents.push_back(v1);
                                        d.contents[j] = data3[j];
                                    }

                                    TLOG() << "====> Trigger_number send: "
                                           << config.trigger_number;
                                    TLOG() << "====> run number send: "
                                           << run_number << "\n";
                                    TLOG()
                                        << "====> record_header_dataset send: "
                                        << path << "\n";
                                    TLOG()
                                        << "Print first 20 entries of a frame "
                                        << nframes;
                                    for (auto ii = 0; ii < nframes; ii++) {
                                        if (ii < 20)
                                            TLOG() << "Sender contents =>"
                                                   << d.contents[ii];
                                    }

                                    info->sender->try_send(
                                        std::move(d),
                                        std::chrono::milliseconds(
                                            config.send_interval_ms));
                                    ++info->messages_sent;
                                    {
                                        std::lock_guard<std::mutex> lk(
                                            tracking_mutex);
                                        if ((completed_receiver_tracking.count(
                                                 info->group_id) &&
                                             completed_receiver_tracking
                                                 [info->group_id]
                                                     .count(info->conn_id)) ||
                                            completed_receiver_tracking.count(
                                                -1)) {
                                            complete_received = true;
                                        }
                                    }
                                    //            if (!check_subscriber()) {
                                    //              TLOG_DEBUG(7) << "Subscriber
                                    //              app has gone away.";
                                    //    complete_received = true;
                                    //            }

                                }  // fragments for loop

                                // wait for the next TR request
                                std::atomic<
                                    std::chrono::steady_clock::time_point>
                                    last_received =
                                        std::chrono::steady_clock::now();
                                while (std::chrono::duration_cast<
                                           std::chrono::milliseconds>(
                                           std::chrono::steady_clock::now() -
                                           last_received.load())
                                           .count() < 500) {
                                    dunedaq::datafilter::Handshake recv;
                                    recv = init_receiver->receive(
                                        Receiver::s_block);
                                    TLOG() << "recv.msg_id " << recv.msg_id;
                                    std::this_thread::sleep_for(100ms);
                                    if (recv.msg_id == "wait") {
                                        continue;
                                    } else if (recv.msg_id == "next_tr") {
                                        TLOG() << "Got next_tr instruction";
                                        break;
                                    }
                                }
                            }
                            // force the while loop to end when no trigger path
                            // left.
                            complete_received = true;
                        }  // par while loop
                    }));
            });

        TLOG_DEBUG(7) << "Joining send threads";
        for (auto& sender : publishers) {
            sender->send_thread->join();
            sender->send_thread.reset(nullptr);
        }
    }
};

}  // namespace trdispatcher
DUNE_DAQ_SERIALIZABLE(dunedaq::datafilter::Data, "data_t");
DUNE_DAQ_SERIALIZABLE(dunedaq::datafilter::Handshake, "init_t");
}  // namespace dunedaq
