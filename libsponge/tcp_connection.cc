#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return count_time_receiver; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    count_time_receiver = 0;
    if (seg.header().rst) {
        set_error();
        return;
    }
    _receiver.segment_received(seg);
    if (seg.header().ack)
        _sender.ack_received(seg.header().ackno, seg.header().win);
    _sender.fill_window();

    /*
        std::cout << " len: " << _sender.segments_out().size();
        std::cout << " re_ackno: " << _receiver.ackno().value();
        std::cout << " seg_seqno: " << seg.header().seqno << std::endl;
        */

    // std::cout << " unbytes: " << unassembled_bytes() << std::endl;

    // send ack for new data
    if (_sender.segments_out().empty()) {
    /*
        bool c1 = (_receiver.ackno().value().raw_value() > seg.header().seqno.raw_value());
        bool c2 = (seg.header().seqno.raw_value() >=
                   (_receiver.ckpoint() + _cfg.recv_capacity + _receiver.isn().raw_value()));
        if (c1 || c2)
        */
//        bool c1 = (_receiver.isn() != seg.header().seqno);
        bool c2 = seg.payload().size();
        bool c3 = seg.header().syn || seg.header().fin;
        if(c2 || c3)
            _sender.send_empty_segment();
    }

    while (!_sender.segments_out().empty())
        _segments_out.push(segment_to_send());

    test_stop_connection();
}

bool TCPConnection::active() const { return is_active; }

size_t TCPConnection::write(const string &data) {
    size_t res;
    res = _sender.stream_in().write(data);
    _sender.fill_window();
    return res;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    count_time_receiver += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    if (_sender.consecutive_retransmissions() == TCPConfig::MAX_RETX_ATTEMPTS) {
        _sender.fill_window();
        if (_sender.segments_out().empty())
            _sender.send_empty_segment();
        TCPSegment t = segment_to_send();
        t.header().rst = true;
        _segments_out.push(t);
//        set_error();
    }
    test_stop_connection();
    while (!_sender.segments_out().empty()) {
        _segments_out.push(segment_to_send());
    }
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    while (!_sender.segments_out().empty())
        _segments_out.push(segment_to_send());
    test_stop_connection();
}

void TCPConnection::connect() {
    _sender.fill_window();
    TCPSegment t = _sender.segments_out().front();
    _sender.segments_out().pop();
    _segments_out.push(t);
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            // Your code here: need to send a RST segment to the peer
            _sender.fill_window();
            if (_sender.segments_out().empty())
                _sender.send_empty_segment();
            TCPSegment t = segment_to_send();
            t.header().rst = true;
            _segments_out.push(t);
            set_error();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

TCPSegment TCPConnection::segment_to_send() {
    TCPSegment t = _sender.segments_out().front();
    _sender.segments_out().pop();
    if (_receiver.ackno().has_value()) {
        t.header().ack = true;
        t.header().ackno = _receiver.ackno().value();
    }
    t.header().win = _receiver.window_size();
    return t;
}

void TCPConnection::set_error() {
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    is_active = false;
}

void TCPConnection::test_stop_connection() {
    if (_receiver.stream_out().eof() && !_sender.stream_in().eof()) {
        _linger_after_streams_finish = false;
    }
    bool p1 = (_receiver.stream_out().eof() && (_receiver.unassembled_bytes() == 0));
    bool p3 = ((_sender.bytes_in_flight() == 0) && _sender.stream_in().eof());
    if (p1 && p3 && !_linger_after_streams_finish) {
        is_active = false;
    }
    if (count_time_receiver >= 10 * _cfg.rt_timeout) {
        is_active = false;
    }
}
