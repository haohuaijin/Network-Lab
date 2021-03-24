#include "stream_reassembler.hh"
#include <vector>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : buffer(),_output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if(eof) eof_index = index+data.size();
    if(index_record == eof_index) _output.end_input();
    std::vector<size_t> mark_erase;

    for(auto &item : buffer){
        if(index <= item.first && index+data.size() > item.first+item.second.size()){
            mark_erase.push_back(item.first);
            total_bytes -= item.second.size();
        } else if(index >= item.first && index+data.size() <= item.first+item.second.size())
            return; 
    }
    for(auto &k : mark_erase) if(buffer.count(k) == 1) buffer.erase(buffer.find(k));

    helper(data, index);

    std::vector<size_t> record_erase;
    for(auto &i : buffer){
        if(i.first < index_record){
            helper(i.second, i.first);
            record_erase.push_back(i.first);
            total_bytes -= buffer[i.first].size();
        } else if(i.first == index_record) {
            while(buffer.count(index_record) == 1){
                 _output.write(buffer[index_record]);
                 total_bytes -= buffer[index_record].size();
                 index_record += buffer[index_record].size();
                 if(index_record == eof_index) _output.end_input();
                 record_erase.push_back(index_record);
            }
        } 
    }
    for(auto &k : record_erase) if(buffer.count(k) == 1)  buffer.erase(buffer.find(k));
}

void StreamReassembler::helper(const string &data, const size_t index){
    size_t len = _output.remaining_capacity();
    if(index > index_record){ // index 大于当前写入 stream 的 index_record
        if(buffer.count(index) == 1){
            if(buffer[index].size() < data.size()){
                total_bytes += data.size() - buffer[index].size();
                buffer[index] = data;
            }
        } else {
            buffer.insert({index, data});
            total_bytes += data.size();
        }
    } else if (index == index_record) { // index 等于当前写入 stream 的 index_record
        _output.write(data);
        index_record += data.size() < len ? data.size() : len;
    } else { // index 小于当前写入 stream 的 index_record
        if(index + data.size() > index_record){
            size_t diff_len = data.size() + index - index_record;
            _output.write(data.substr(index_record-index, diff_len));
            index_record += diff_len < len ? diff_len : len; 
        }
    }
    if(index_record == eof_index) _output.end_input();
}

size_t StreamReassembler::unassembled_bytes() const { return total_bytes; }

bool StreamReassembler::empty() const { return total_bytes == 0; }
