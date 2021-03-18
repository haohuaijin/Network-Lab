#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity): 
    cap(capacity, '\0'), size(capacity), top(0) {}

size_t ByteStream::write(const string &data) {
    size_t len = data.size();
    if(len + top > size) return 0;
    else {
        // rewrite to insert method
        for(int i=0; i<len; i++)
            cap[top+i] = data[i];
        top += len;
    } 
    sum_write += len;
    return len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    return cap.substr(0, len);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    int temp = top;
    for(int i=0; i<top-len; i++){
        cap[i] = cap[len+i];
    }    
    top -= len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string res = peek_output(len);
    pop_output(len);
    sum_read += len;
    return res;
}

void ByteStream::end_input() { end_flag = true; }

bool ByteStream::input_ended() const { return end_flag; }

size_t ByteStream::buffer_size() const { return top; }

bool ByteStream::buffer_empty() const { return top == 0; }

bool ByteStream::eof() const { return false; }

size_t ByteStream::bytes_written() const { return sum_write; }

size_t ByteStream::bytes_read() const { return sum_read; }

size_t ByteStream::remaining_capacity() const { return size - top; }
