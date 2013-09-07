using namespace std;

class Packet {
    public:
        // time packet should be sent
        int timestamp;
        string contents;
        Packet(int i = 0, string j = ""):timestamp(i), contents(j) {};
        int get_timestamp() const {return timestamp;}
        string get_content() const {return contents;}
};
