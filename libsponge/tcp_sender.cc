#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include <iostream>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : Rto{retx_timeout}
    , _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return fbytes; }

void TCPSender::fill_window() {
    if(!is_send_syn){ send_syn(); return; }
    size_t buffer_len = _stream.buffer_size();
    if(buffer_len == 0 && !_stream.input_ended()) return;
    //TCPConfig::MAX_PAYLOAD_SIZE
    while(wSize > TCPConfig::MAX_PAYLOAD_SIZE && buffer_len > TCPConfig::MAX_PAYLOAD_SIZE){
        TCPSegment t;     
        send_segments(t, TCPConfig::MAX_PAYLOAD_SIZE);
        buffer_len -= TCPConfig::MAX_PAYLOAD_SIZE;
    }
    //这个条件需要修改
    if((wSize == 0 && fbytes == 0) && !_stream.input_ended()){
        send_empty_segment();
    }else if(wSize > buffer_len){
        TCPSegment t;
        if(_stream.input_ended()) {
            send_segments(t, buffer_len+1, false, true);
        } else {
            send_segments(t, buffer_len);
        }
    } else if(wSize <= buffer_len && wSize) {
        TCPSegment t;
        send_segments(t, wSize);
    }
    if(!rTimer.is_run() && fbytes != 0)
        rTimer.start(Rto);
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    wSize = window_size;
    con_retran = 0;
    Rto = _initial_retransmission_timeout;
    rTimer.stop();
    uint64_t abs_ackno = unwrap(ackno, _isn, _next_seqno);
    // 这里的buffer[0].first+buffer[0].second.length_in_sequence_space()等于下一个序列号。 
    while(buffer.size() > 0 && (buffer[0].first+buffer[0].second.length_in_sequence_space() <= abs_ackno)){
        fbytes -= buffer[0].second.length_in_sequence_space();
        buffer.erase(buffer.begin());
    }
    if(fbytes != 0) rTimer.start(Rto);
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    rTimer.elapsed(ms_since_last_tick);
    if(!rTimer.is_run()){
    /*
        for(auto i : buffer)
            _segments_out.push(i.second);
     */
        _segments_out.push(buffer[0].second);
        con_retran += 1;
        Rto *= 2;
        rTimer.start(Rto);
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return con_retran; }

void TCPSender::send_empty_segment() {
    TCPSegment t;
    t.header().seqno = next_seqno();
    _segments_out.push(t);
}

void TCPSender::send_syn(){
    is_send_syn = true;
    TCPSegment t;     
    send_segments(t, 1, true, false);
    if(!rTimer.is_run() && fbytes != 0)
        rTimer.start(Rto);
}

void TCPSender::send_segments(TCPSegment& t, uint64_t len, bool is_syn, bool is_fin){
    if(is_syn) t.header().syn = true;
    if(is_fin) t.header().fin = true;
    t.header().seqno = next_seqno();
    t.payload() = Buffer(_stream.read((is_syn||is_fin) ? len-1 : len));
    _segments_out.push(t);                       //send to peer
    buffer.push_back(make_pair(_next_seqno, t)); //store for retransmission
    wSize -= len;
    fbytes += len;
    _next_seqno += len;
}

