#ifndef DFBACKEND_INCLUDE_HDF5FromStorage_HPP_
#define DFBACKEND_INCLUDE_HDF5FromStorage_HPP_

#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include "logging/Logging.hpp"
#include "nlohmann/json.hpp"

namespace dunedaq {
namespace datafilter {

struct HDF5FromStorage {
    nlohmann::json hdf5_files_json;
    std::vector<nlohmann::json> hdf5_files_already_transfer;
    std::vector<std::filesystem::path> hdf5_files_to_transfer;
    std::vector<std::filesystem::path> hdf5_files_waiting;
    // const std::string json_file="hdf5_files_list.json";
    const std::string json_file;
    const std::string storage_pathname;

    HDF5FromStorage(const std::string& storage_pathname,
                    const std::string json_file)
        : storage_pathname(storage_pathname), json_file(json_file) {
        ReadWriteJSON();
        HDF5Find();
    }

    void HDF5Find() {
        const std::filesystem::path daq_storage_path{storage_pathname};
        for (auto const& entry :
             std::filesystem::directory_iterator{daq_storage_path}) {
            // std::cout << entry.path().filename().string() << '\n';
            // std::cout << entry.path().extension().string() << '\n';

            std::string file_ext = entry.path().extension().string();

            if (file_ext == ".hdf5") {
                TLOG() << "found a new hdf5 file: "
                       << entry.path().filename().string() << '\n';
                for (auto item : hdf5_files_already_transfer) {
                    TLOG() << "item" << item << "\n";
                    if (item != entry.path().filename().string()) {
                        TLOG() << "transfer" << entry.path().filename().string()
                               << '\n';
                        hdf5_files_to_transfer.push_back(entry.path());
                        break;
                    }
                }
            }
            if (file_ext == ".writing") {
                hdf5_files_waiting.push_back(entry.path());
                // std::cout << "found waiting list
                // :"<<entry.path().filename().string() << '\n';
            }
        }

        std::sort(hdf5_files_to_transfer.begin(), hdf5_files_to_transfer.end());
        hdf5_files_to_transfer.erase(unique(hdf5_files_to_transfer.begin(),
                                            hdf5_files_to_transfer.end()),
                                     hdf5_files_to_transfer.end());
    }
    void ReadWriteJSON() {
        std::ifstream file_in(json_file);
        file_in >> hdf5_files_json;
        file_in.close();
        auto hdf5_files1 = hdf5_files_json["hdf5_files"];
        for (auto hdf5_file : hdf5_files1) {
            // std::cout<<"hdf5_file
            // "<<hdf5_file["hdf5_file"].get<std::string>()<<"\n"; std::string
            // hdf5_file1 = hdf5_file["hdf5_file"].get<std::string()>;
            hdf5_files_already_transfer.push_back(hdf5_file["hdf5_file"]);
        }
        // remove duplicate in the json file
        std::sort(hdf5_files_already_transfer.begin(),
                  hdf5_files_already_transfer.end());
        hdf5_files_already_transfer.erase(
            unique(hdf5_files_already_transfer.begin(),
                   hdf5_files_already_transfer.end()),
            hdf5_files_already_transfer.end());
        save(json_file);
    }

    void save(const std::string json_file) {
        std::ofstream file_out(json_file);
        nlohmann::json j, new_entry;
        j = hdf5_files_json;
        // new_entry["hdf5_file"]="coko.hdf5";
        ////j.insert(j1.begin(),j1.end());
        // j["hdf5_files"].push_back(new_entry);

        // j["hdf5_files"]["hdf5_file"]="coko.hdf5";
        file_out << std::setw(4) << j << std::endl;

        file_out.close();
    }

    void print() {
        std::cout << "print info"
                  << "\n";
        for (auto file : hdf5_files_to_transfer) {
            std::cout << "HDF5 file to transfer" << file << "\n";
        }
        for (auto file : hdf5_files_waiting) {
            std::cout << "HDF5 file waiting" << file << "\n";
        }
        for (auto file : hdf5_files_already_transfer) {
            std::cout << "HDF5 file already transfer" << file << "\n";
        }
    }
};

// struct ReadWriteJSON
//{
//     ReadWriteJSON(const std::string json_file)
//     {
//
//     auto hdf5_files_already_transfer = std::vector<nlohmann::json> {};
//     //const std::string json_file="hdf5_files_list.json";
//     nlohmann::json hdf5_files_json;
//     std::ifstream file_in(json_file);
//     file_in >> hdf5_files_json;
//     file_in.close();
//     auto hdf5_files1=hdf5_files_json["hdf5_files"];
//     for (auto hdf5_file : hdf5_files1)
//     {
//         std::cout<<"hdf5_file
//         "<<hdf5_file["hdf5_file"].get<std::string>()<<"\n";
//         //std::string hdf5_file1 = hdf5_file["hdf5_file"].get<std::string()>;
//         hdf5_files_already_transfer.push_back(hdf5_file["hdf5_file"]);
//     }
//
//     //void save(const std::string json_file)
//     //return hdf5_files_already_transfer;
//     }
// };

// struct JFileManager
//     static void save(const ReadWriteJSON& rwj,const std::string json_file)
//     {
//         std::ofstream file_out(json_file);
//         nlohmann::json j,new_entry;
//         j = hdf5_files_json;
//         new_entry["hdf5_file"]="coko.hdf5";
//         //j.insert(j1.begin(),j1.end());
//         j["hdf5_files"].push_back(new_entry);
//
//         //j["hdf5_files"]["hdf5_file"]="coko.hdf5";
//         file_out<< std::setw(4)<<j <<std::endl;
//
//         file_out.close();
//     }
// }

// struct HDF5ReaderFromStorage1
//{
//     auto hdf5_files_to_transfer = std::vector<std::filesystem::path> {};
//     auto hdf5_files_waiting = std::vector<std::filesystem::path> {};
//     auto hdf5_files_already_transfer = std::vector<nlohmann::json> {};
//
//     const std::string json_file="hdf5_files_list.json";
//     nlohmann::json hdf5_files_json;
//     std::ifstream file_in(json_file);
//     file_in >> hdf5_files_json;
//     file_in.close();
//
//     auto hdf5_files1=hdf5_files_json["hdf5_files"];
//     for (auto hdf5_file : hdf5_files1)
//     {
//         std::cout<<"hdf5_file
//         "<<hdf5_file["hdf5_file"].get<std::string>()<<"\n";
//         //std::string hdf5_file1 = hdf5_file["hdf5_file"].get<std::string()>;
//         hdf5_files_already_transfer.push_back(hdf5_file["hdf5_file"]);
//     }
//
//     const std::filesystem::path
//     daq_storage_dir{"/lcg/storage19/test-area/dune-v4-spack-integration2/sourcecode/daqconf/config/"};
//     std::regex ext1(".hdf5");
//
//     std::string ext=".hdf5";
//
//     for (auto const& entry :
//     std::filesystem::directory_iterator{daq_storage_dir})
//     {
//        //std::cout << entry.path().filename().string() << '\n';
//        //std::cout << entry.path().extension().string() << '\n';
//
//        std::string file_ext = entry.path().extension().string();
//
//        if (file_ext == ".hdf5")
//        {
//            for (auto item : hdf5_files_already_transfer )
//            {
//
//                if (item != entry.path().filename().string()){
//                     hdf5_files_to_transfer.push_back(entry.path());
//                     hdf5_files_json["hdf5_file"]="coko_file.hdf5";
//                }
//            }
//           std::cout << "found list of hdf5 files to datafilter:
//           "<<entry.path().filename().string() << '\n';
//        }
//        if (file_ext == ".writing")
//        {
//           hdf5_files_waiting.push_back(entry.path());
//           std::cout << "found waiting list
//           :"<<entry.path().filename().string() << '\n';
//        }
//     }
//
//     std::ofstream file_out("test.json");
//     nlohmann::json j,new_entry;
//     j = hdf5_files_json;
//     new_entry["hdf5_file"]="coko.hdf5";
//     //j.insert(j1.begin(),j1.end());
//     j["hdf5_files"].push_back(new_entry);
//
//     //j["hdf5_files"]["hdf5_file"]="coko.hdf5";
//     file_out<< std::setw(4)<<j <<std::endl;
//
//     file_out.close();
//
//     for (auto file : hdf5_files_to_transfer)
//     {
//         std::cout <<file <<"\n";
//     }
//
//
// }

}  // namespace datafilter
}  // namespace dunedaq
#endif
