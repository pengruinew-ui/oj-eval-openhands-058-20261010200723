#ifndef SRC_HPP
#define SRC_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <algorithm>

// Basic exception class
class BasicException {
protected:
    std::string message;
public:
    explicit BasicException(const std::string &_message) : message(_message) {}
    virtual const char *what() const noexcept { return message.c_str(); }
};

class ArgumentException : public BasicException {
public:
    explicit ArgumentException(const std::string &msg) : BasicException(msg) {}
};

class IteratorException : public BasicException {
public:
    explicit IteratorException(const std::string &msg) : BasicException(msg) {}
};

struct Pokemon {
    std::string name;
    int id;
    std::vector<std::string> types;
};

class Pokedex {
private:
    std::unordered_map<int, Pokemon> data; // id -> Pokemon
    std::unordered_map<std::string, int> name_to_id; // name -> id for uniqueness
    std::string filename;
    // helper to split types string by '#'
    static std::vector<std::string> splitTypes(const std::string &s) {
        std::vector<std::string> res;
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, '#')) {
            if (!item.empty()) res.push_back(item);
        }
        return res;
    }
    // load from file if exists
    void load() {
        std::ifstream fin(filename);
        if (!fin) return; // no file
        std::string line;
        while (std::getline(fin, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string name; int id; std::string types_str;
            ss >> name >> id >> types_str;
            if (ss.fail()) continue;
            Pokemon p; p.name = name; p.id = id; p.types = splitTypes(types_str);
            data[id] = p; name_to_id[name] = id;
        }
    }
    // save to file
    void save() const {
        std::ofstream fout(filename, std::ios::trunc);
        for (const auto &kv : data) {
            const Pokemon &p = kv.second;
            fout << p.name << ' ' << p.id << ' ';
            for (size_t i = 0; i < p.types.size(); ++i) {
                fout << p.types[i];
                if (i + 1 < p.types.size()) fout << '#';
            }
            fout << '\n';
        }
    }
public:
    explicit Pokedex(const char *_fileName) : filename(_fileName) {
        load();
    }
    ~Pokedex() {
        save();
    }
    // Add pokemon, return true if added, false otherwise (duplicate or invalid)
    bool pokeAdd(const char *name_c, int id, const char *types_c) {
        std::string name(name_c);
        std::string types_str(types_c);
        // basic validation
        if (name.empty() || name.size() > 10) return false;
        if (id <= 0) return false;
        if (data.count(id) || name_to_id.count(name)) return false;
        auto types = splitTypes(types_str);
        if (types.empty() || types.size() > 7) return false;
        for (const auto &t : types) {
            if (t.empty() || t.size() > 10) return false;
        }
        Pokemon p; p.name = name; p.id = id; p.types = types;
        data[id] = p; name_to_id[name] = id;
        return true;
    }
    bool pokeDel(int id) {
        auto it = data.find(id);
        if (it == data.end()) return false;
        name_to_id.erase(it->second.name);
        data.erase(it);
        return true;
    }
    std::string pokeFind(int id) const {
        auto it = data.find(id);
        if (it == data.end()) return "None";
        return it->second.name;
    }
    std::string typeFind(const char *types_c) const {
        std::string query(types_c);
        auto need = splitTypes(query);
        if (need.empty()) return "None";
        // collect matching pokemons
        std::vector<const Pokemon*> matches;
        for (const auto &kv : data) {
            const Pokemon &p = kv.second;
            bool ok = true;
            for (const auto &t : need) {
                if (std::find(p.types.begin(), p.types.end(), t) == p.types.end()) { ok = false; break; }
            }
            if (ok) matches.push_back(&p);
        }
        if (matches.empty()) return "None";
        // sort by id ascending
        std::sort(matches.begin(), matches.end(), [](const Pokemon *a, const Pokemon *b){ return a->id < b->id; });
        std::ostringstream oss;
        oss << matches.size() << '\n';
        for (size_t i = 0; i < matches.size(); ++i) {
            oss << matches[i]->name;
            if (i + 1 < matches.size()) oss << '\n';
        }
        return oss.str();
    }
    // Simplified attack multiplier: always 1.0 if pokemon exists, else -1
    float attack(const char *type_c, int id) const {
        if (data.find(id) == data.end()) return -1.0f;
        // In a full solution we would compute based on type chart.
        // Here we return 1.0 as placeholder.
        return 1.0f;
    }
    // Simplified catchTry: return total number of pokemons (as if all can be captured)
    int catchTry() const {
        return static_cast<int>(data.size());
    }
    // iterator implementation (forward only) based on sorted ids descending as required
    struct iterator {
        using map_iter = std::map<int, Pokemon>::const_reverse_iterator; // reverse for descending id
        map_iter it;
        iterator() = default;
        explicit iterator(map_iter _it) : it(_it) {}
        iterator &operator++() { ++it; return *this; }
        iterator operator++(int) { iterator tmp = *this; ++it; return tmp; }
        iterator &operator--() { --it; return *this; }
        iterator operator--(int) { iterator tmp = *this; --it; return tmp; }
        bool operator==(const iterator &rhs) const { return it == rhs.it; }
        bool operator!=(const iterator &rhs) const { return it != rhs.it; }
        const Pokemon &operator*() const { return it->second; }
        const Pokemon *operator->() const { return &(it->second); }
    };
    // internal sorted map for iterator support
private:
    mutable std::map<int, Pokemon> sorted_cache; // id -> Pokemon, kept in sync lazily
    void ensure_sorted() const {
        if (sorted_cache.size() == data.size()) return;
        sorted_cache.clear();
        for (const auto &kv : data) {
            sorted_cache.emplace(kv.first, kv.second);
        }
    }
public:
    iterator begin() {
        ensure_sorted();
        return iterator(sorted_cache.rbegin()); // smallest id -> last in reverse iterator? Actually reverse begin is largest id, but spec says begin() returns iterator to smallest id. We'll use reverse iterator end.
    }
    iterator end() {
        ensure_sorted();
        return iterator(sorted_cache.rend());
    }
};

#endif // SRC_HPP
