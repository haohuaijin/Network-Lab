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
    if(len + top > size) len = size - top;
    // 队列，后面入队列，前面出队列
    for(size_t i=0; i<len; i++)
        cap[top+i] = data[i];
    top += len;
    sum_write += len;
    return len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    return cap.substr(0, len);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    // 出队列，需要循环讲后面的元素提到前面来
    // 这里没有检测元素出队列的元素是否多于队列中的元素
    // 这里也可以使用循环数组，来节省数组复制的开销
    for(size_t i=0; i<top-len; i++){
        cap[i] = cap[len+i];
    }    
    top -= len;
    sum_read += len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    std::string res = peek_output(len);
    pop_output(len);
    return res;
}

void ByteStream::end_input() { end_flag = true; }

bool ByteStream::input_ended() const { return end_flag; }

size_t ByteStream::buffer_size() const { return top; }

bool ByteStream::buffer_empty() const { return top == 0; }

bool ByteStream::eof() const { return end_flag && (top == 0); }

size_t ByteStream::bytes_written() const { return sum_write; }

size_t ByteStream::bytes_read() const { return sum_read; }

size_t ByteStream::remaining_capacity() const { return size - top; }
