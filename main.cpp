#include <bits/stdc++.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

using namespace std;

// Persistent KV store: uses 8 bucket files under ./kvdb/
// Each record: [op:1][key_len:1][value:int32_le][key_bytes:key_len]
// op: 0 = insert, 1 = delete
// Key length <= 64 per spec

static inline uint64_t fnv1a64(const string &s) {
    const uint64_t FNV_offset = 1469598103934665603ull;
    const uint64_t FNV_prime  = 1099511628211ull;
    uint64_t h = FNV_offset;
    for (unsigned char c : s) {
        h ^= (uint64_t)c;
        h *= FNV_prime;
    }
    return h;
}

static inline int bucket_id(const string &key) {
    uint64_t h = fnv1a64(key);
    return (int)(h & 7ull); // 8 buckets: 0..7
}

static inline string bucket_path(int id) {
    char buf[64];
    snprintf(buf, sizeof(buf), "kvdb/bucket_%d.dat", id);
    return string(buf);
}

static void ensure_db_init() {
    struct stat st{};
    if (stat("kvdb", &st) != 0) {
        // create directory
        #ifdef _WIN32
        _mkdir("kvdb");
        #else
        mkdir("kvdb", 0755);
        #endif
    }
    // Create empty bucket files if not exist
    for (int i = 0; i < 8; ++i) {
        string p = bucket_path(i);
        FILE *f = fopen(p.c_str(), "ab");
        if (f) fclose(f);
    }
}

static inline void write_record(FILE *f, uint8_t op, const string &key, int32_t value) {
    uint8_t klen = (uint8_t)key.size();
    fwrite(&op, 1, 1, f);
    fwrite(&klen, 1, 1, f);
    // write int32 little-endian explicitly
    uint32_t v = (uint32_t)value;
    unsigned char b[4];
    b[0] = (unsigned char)(v & 0xFF);
    b[1] = (unsigned char)((v >> 8) & 0xFF);
    b[2] = (unsigned char)((v >> 16) & 0xFF);
    b[3] = (unsigned char)((v >> 24) & 0xFF);
    fwrite(b, 1, 4, f);
    if (klen) fwrite(key.data(), 1, klen, f);
}

static inline bool read_record(FILE *f, uint8_t &op, string &key, int32_t &value) {
    unsigned char header[6];
    size_t n = fread(header, 1, 6, f);
    if (n == 0) return false; // EOF
    if (n < 6) return false;  // partial/corrupt, treat as EOF
    op = header[0];
    uint8_t klen = header[1];
    uint32_t v = (uint32_t)header[2] | ((uint32_t)header[3] << 8) | ((uint32_t)header[4] << 16) | ((uint32_t)header[5] << 24);
    value = (int32_t)v;
    key.resize(klen);
    if (klen) {
        size_t m = fread(&key[0], 1, klen, f);
        if (m < klen) return false; // partial
    }
    return true;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    ensure_db_init();

    int n;
    if (!(cin >> n)) return 0;

    string cmd, index;
    for (int i = 0; i < n; ++i) {
        cin >> cmd;
        if (cmd == "insert") {
            int v; cin >> index >> v;
            int bid = bucket_id(index);
            string p = bucket_path(bid);
            FILE *f = fopen(p.c_str(), "ab");
            if (!f) return 0; // fail quietly per OJ
            write_record(f, 0u, index, (int32_t)v);
            fclose(f);
        } else if (cmd == "delete") {
            int v; cin >> index >> v;
            int bid = bucket_id(index);
            string p = bucket_path(bid);
            FILE *f = fopen(p.c_str(), "ab");
            if (!f) return 0;
            write_record(f, 1u, index, (int32_t)v);
            fclose(f);
        } else if (cmd == "find") {
            cin >> index;
            int bid = bucket_id(index);
            string p = bucket_path(bid);
            FILE *f = fopen(p.c_str(), "rb");
            if (!f) {
                cout << "null\n";
                continue;
            }
            uint8_t op; string key; int32_t val;
            // Track presence for this key only
            // values are non-negative; but using unordered_set<int> is fine
            unordered_set<int> present;
            present.reserve(64);
            while (read_record(f, op, key, val)) {
                if (key == index) {
                    if (op == 0u) {
                        present.insert((int)val);
                    } else if (op == 1u) {
                        auto it = present.find((int)val);
                        if (it != present.end()) present.erase(it);
                        // If deletion before insertion, ignore
                    }
                }
            }
            fclose(f);
            if (present.empty()) {
                cout << "null\n";
            } else {
                vector<int> vals; vals.reserve(present.size());
                for (int x : present) vals.push_back(x);
                sort(vals.begin(), vals.end());
                for (size_t j = 0; j < vals.size(); ++j) {
                    if (j) cout << ' ';
                    cout << vals[j];
                }
                cout << '\n';
            }
        } else {
            // Unknown command; consume rest of line
            string rest; getline(cin, rest);
        }
    }
    return 0;
}

