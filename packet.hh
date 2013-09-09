using namespace std;

class Packet {
    public:
        // time packet should be sent
        uint64_t timestamp;
        string contents;
        Packet(uint64_t i = 0, string j = ""):timestamp(i), contents(j) {};
        uint64_t get_timestamp() const {return timestamp;}
        string get_content() const {return contents;}
};
