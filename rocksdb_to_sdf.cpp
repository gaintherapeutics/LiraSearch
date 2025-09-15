#include <iostream>
#include <fstream>
#include <string>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <filesystem>

void save_to_file(const std::string& filename, const std::string& content) {
    std::ofstream out(filename, std::ios::out | std::ios::binary);
    if (!out) {
        throw std::runtime_error("Failed to open output file: " + filename);
    }
    out.write(content.c_str(), content.size());
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <rocksdb_path> <keys_file.txt> <output_folder>\n";
        return 1;
    }

    std::string db_path = argv[1];
    std::string keys_file_path = argv[2];
    std::string output_folder = argv[3];

    // Check if output folder exists
    if (!std::filesystem::exists(output_folder)) {
        std::cerr << "Output folder does not exist: " << output_folder << "\n";
        return 1;
    }

    // Open RocksDB in read-only mode
    rocksdb::DB* db;
    rocksdb::Options options;
    options.create_if_missing = false;

    rocksdb::Status status = rocksdb::DB::OpenForReadOnly(options, db_path, &db);
    if (!status.ok()) {
        std::cerr << "Failed to open RocksDB in read-only mode: " << status.ToString() << "\n";
        return 1;
    }

    std::ifstream keys_file(keys_file_path);
    if (!keys_file) {
        std::cerr << "Could not open keys file: " << keys_file_path << "\n";
        delete db;
        return 1;
    }

    std::string key;
    while (std::getline(keys_file, key)) {
        if (key.empty()) continue;

        std::string value;
        rocksdb::Status s = db->Get(rocksdb::ReadOptions(), key, &value);

        if (!s.ok()) {
            std::cerr << "Key not found or read error for '" << key << "': " << s.ToString() << "\n";
            continue;
        }

        try {
            std::string output_path = output_folder + "/" + key;
            save_to_file(output_path, value);
            std::cout << "Recovered: " << output_path << "\n";
        } catch (const std::exception& ex) {
            std::cerr << "Failed to save file for key '" << key << "': " << ex.what() << "\n";
        }
    }

    delete db;
    return 0;
}
