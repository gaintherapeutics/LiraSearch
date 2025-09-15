#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>

#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/table.h>
#include <rocksdb/utilities/options_util.h>

namespace fs = std::filesystem;

// Read keys from a text file (one key per line)
std::vector<std::string> read_keys(const std::string& key_file_path) {
    std::ifstream infile(key_file_path);
    if (!infile) {
        throw std::runtime_error("Failed to open key file: " + key_file_path);
    }

    std::vector<std::string> keys;
    std::string line;
    while (std::getline(infile, line)) {
        if (!line.empty()) {
            keys.push_back(line);
        }
    }
    return keys;
}

// Write file content to output directory
void write_file(const fs::path& output_dir, const std::string& filename, const std::string& content) {
    fs::path out_path = output_dir / filename;
    std::ofstream outfile(out_path, std::ios::binary);
    if (!outfile) {
        throw std::runtime_error("Failed to write file: " + out_path.string());
    }
    outfile.write(content.data(), content.size());
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <rocksdb_path> <key_file.txt> <output_directory>\n";
        return 1;
    }

    std::string db_path = argv[1];
    std::string key_file = argv[2];
    fs::path output_dir = argv[3];

    fs::create_directories(output_dir);  // Create output folder if missing

    // --------------- RocksDB OPTIONS: Minimal RAM Usage ---------------
    rocksdb::DB* db = nullptr;
    rocksdb::Options options;
	rocksdb::BlockBasedTableOptions table_options;

	table_options.no_block_cache = true;

    options.create_if_missing = false;
    options.max_open_files = 100;
    options.allow_mmap_reads = false;
    options.allow_mmap_writes = false;
    options.use_direct_reads = false;
    options.use_direct_io_for_flush_and_compaction = false;   
	options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));


    // Open RocksDB in read-only mode
    rocksdb::Status status = rocksdb::DB::OpenForReadOnly(options, db_path, &db);
    if (!status.ok()) {
        std::cerr << "Failed to open RocksDB (read-only): " << status.ToString() << "\n";
        return 1;
    }

    // Lightweight read options
    rocksdb::ReadOptions read_options;
    read_options.fill_cache = false;        // Do not pollute block cache
    read_options.verify_checksums = false;  // Skip checksum verification

    // --------------- Key Lookup and Restore ---------------
    size_t success_count = 0;
    size_t fail_count = 0;

    try {
        auto keys = read_keys(key_file);

        for (const auto& key : keys) {
            std::string value;
            rocksdb::Status s = db->Get(read_options, key, &value);
            if (s.ok()) {
                try {
                    write_file(output_dir, key, value);
                    std::cout << "Restored: " << key << "\n";
                    ++success_count;
                } catch (const std::exception& e) {
                    std::cerr << "Write error [" << key << "]: " << e.what() << "\n";
                    ++fail_count;
                }
            } else {
                std::cerr << "Key not found or error: " << key << " - " << s.ToString() << "\n";
                ++fail_count;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        delete db;
        return 1;
    }

    std::cout << "\nRestoration complete.\n";
    std::cout << "Successful: " << success_count << "\n";
    std::cout << "Failed: " << fail_count << "\n";

    delete db;
    return 0;
}
