#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    int len = data.size();
    if(len + total_bytes + _output.remaining_capacity() > _capacity)
        return;
    if(index == index_record){
        _output.write(data);
        index_record++;
    } else {
        buffer.insert({index, data});
        total_bytes += len;
    }
    while(buffer.count(index_record) != 0){
        std::string current_data = buffer[index_record];
        _output.write(current_data);
        total_bytes -= current_data.size();
        buffer.erase(buffer.find(index_record));
        index_record++;
    } 
}

size_t StreamReassembler::unassembled_bytes() const { return total_bytes; }

bool StreamReassembler::empty() const { return total_bytes == 0; }
