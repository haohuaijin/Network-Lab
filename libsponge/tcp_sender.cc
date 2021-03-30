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
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return fbytes; }

void TCPSender::fill_window() {
    if(!is_send_syn){ send_syn(); return; }
    size_t buffer_len = _stream.buffer_size();
    if(buffer_len == 0) return;
    //TCPConfig::MAX_PAYLOAD_SIZE
    while(wSize > TCPConfig::MAX_PAYLOAD_SIZE && buffer_len > TCPConfig::MAX_PAYLOAD_SIZE){
        wSize -= TCPConfig::MAX_PAYLOAD_SIZE;
        buffer_len -= TCPConfig::MAX_PAYLOAD_SIZE;
        TCPSegment t;     
        t.header().seqno = next_seqno();
        t.payload() = Buffer(_stream.read(TCPConfig::MAX_PAYLOAD_SIZE));
        _segments_out.push(t);

        buffer.push_back(make_pair(_next_seqno, t));

        fbytes += TCPConfig::MAX_PAYLOAD_SIZE;
        _next_seqno += TCPConfig::MAX_PAYLOAD_SIZE;
    }
    if(wSize == 0 && fbytes == 0){
        send_empty_segment();
    }else if(wSize >= buffer_len){
        wSize -= buffer_len;
        TCPSegment t;
        t.header().seqno = next_seqno();
        t.payload() = Buffer(_stream.read(buffer_len));
    
        buffer.push_back(make_pair(_next_seqno, t));

        fbytes += buffer_len;
        _next_seqno += buffer_len;
        _segments_out.push(t);
    } else {
        TCPSegment t;
        t.header().seqno = next_seqno();
        t.payload() = Buffer(_stream.read(wSize));
        
        buffer.push_back(make_pair(_next_seqno, t));

        fbytes += wSize;
        _next_seqno += wSize;
        _segments_out.push(t);
        wSize = 0;
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    wSize = window_size;
    uint64_t abs_ackno = unwrap(ackno, _isn, _next_seqno);
    while(buffer.size() > 0 && buffer[0].first <= abs_ackno){
        fbytes -= buffer[0].second.length_in_sequence_space();
        buffer.erase(buffer.begin());
    }
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { DUMMY_CODE(ms_since_last_tick); }

unsigned int TCPSender::consecutive_retransmissions() const { return {}; }

void TCPSender::send_empty_segment() {
    TCPSegment t;
    t.header().seqno = next_seqno();
    _segments_out.push(t);
}

void TCPSender::send_syn(){
    is_send_syn = true;
    TCPSegment t;     
    t.header().syn = true;
    t.header().seqno = _isn;
    fbytes += 1;
    _next_seqno += 1;
    _segments_out.push(t);
    wSize -= 1;
    buffer.push_back(make_pair(0, t));
}
void TCPSender::send_fin(){
    TCPSegment t;     
    t.header().fin = true;
    t.header().seqno = _isn;
    fbytes += 1;
    _next_seqno += 1;
    _segments_out.push(t);
    wSize -= 1;
    buffer.push_back(make_pair(_next_seqno-1, t));
}

