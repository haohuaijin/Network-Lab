#include "stream_reassembler.hh"

#include <vector>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : buffer(), _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (eof)
        eof_index = index + data.size();
    if (index_record == eof_index)
        _output.end_input();
    std::vector<size_t> erase_key;

    // 处理下面这种情况 "c",2,0 "bcd",1,0
    for (auto &i : buffer) {
        if (index <= i.first && index + data.size() > i.first + i.second.size()) {
            erase_key.push_back(i.first);
            total_bytes -= i.second.size();
        } else if (index >= i.first && index + data.size() <= i.first + i.second.size())
            return;
    }
    for (auto &k : erase_key)
        buffer.erase(k);

    helper(data, index);

    // 查看是否有data可以输出到_output
    erase_key.clear();
    for (auto &i : buffer) {
        if (i.first < index_record) {  // 处理"b",1,0; "ab",0,0
            erase_key.push_back(i.first);
            total_bytes -= buffer[i.first].size();
            helper(i.second, i.first);
        } else if (i.first == index_record) {
            while (buffer.count(index_record)) {
                erase_key.push_back(index_record);
                total_bytes -= buffer[index_record].size();
                // 这一行放在最后是因为index_record会在helper里面更改
                helper(buffer[index_record], index_record);
            }
        }
    }
    for (auto &k : erase_key)
        buffer.erase(k);
}

void StreamReassembler::helper(const string &data, const size_t index) {
    size_t len = _output.remaining_capacity();
    if (index > index_record) {  // index 大于当前写入 stream 的 index_record
        if (buffer.count(index)) {
            if (buffer[index].size() < data.size()) {
                total_bytes += data.size() - buffer[index].size();
                buffer[index] = data;
            }
        } else {
            buffer.insert({index, data});
            total_bytes += data.size();
        }
    } else if (index == index_record) {  // index 等于当前写入 stream 的 index_record
        _output.write(data);
        index_record += data.size() < len ? data.size() : len;
    } else {  // index 小于当前写入 stream 的 index_record
        if (index + data.size() > index_record) {
            size_t diff_len = data.size() + index - index_record;
            _output.write(data.substr(index_record - index, diff_len));
            index_record += diff_len < len ? diff_len : len;
        }
    }
    if (index_record == eof_index)
        _output.end_input();
}

size_t StreamReassembler::unassembled_bytes() const { return total_bytes; }

bool StreamReassembler::empty() const { return total_bytes == 0; }
