#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

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
    if (!is_send_syn) {
        send_syn();
        return;
    }
    if (is_send_fin)
        return;
    size_t buffer_len = _stream.buffer_size();
    if (buffer_len == 0 && !_stream.eof())
        return;

    while (wSize > TCPConfig::MAX_PAYLOAD_SIZE && buffer_len > TCPConfig::MAX_PAYLOAD_SIZE) {
        TCPSegment t;
        send_segments(t, TCPConfig::MAX_PAYLOAD_SIZE);
        buffer_len -= TCPConfig::MAX_PAYLOAD_SIZE;
    }

    TCPSegment t;
    // send detective segments
    if ((wSize == 0 && fbytes == 0)) {
        is_detective = true;
        if (_stream.eof()) {
            is_send_fin = true;
            send_segments(t, buffer_len + 1, false, true);
        } else {
            send_segments(t, 1);
        }
    } else if (wSize >= buffer_len + 1) {
        if (_stream.input_ended()) {
            is_send_fin = true;
            send_segments(t, buffer_len + 1, false, true);
        } else {
            send_segments(t, buffer_len);
        }
    } else if (wSize <= buffer_len && wSize) {
        send_segments(t, wSize);
    }
    if (!rTimer.is_run() && fbytes != 0)
        rTimer.start(Rto);
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t abs_ackno = unwrap(ackno, _isn, _next_seqno);

    // reset the is_detective
    if (is_detective)
        is_detective = false;

    // when we receive partially acknowledges, we drop it
    bool flag = false;
    for (auto i : buffer)
        if (abs_ackno == i.first + i.second.length_in_sequence_space())
            flag = true;
    if (abs_ackno > curr_ack && !flag)  // need change
        return;

    // 这里的buffer[0].first+buffer[0].second.length_in_sequence_space()等于下一个序列号。
    while (buffer.size() > 0 && (buffer[0].first + buffer[0].second.length_in_sequence_space() <= abs_ackno)) {
        fbytes -= buffer[0].second.length_in_sequence_space();
        buffer.erase(buffer.begin());
    }
    wSize = window_size;
    if (abs_ackno <= curr_ack)
        return;  // if the ackno is previous, we don't restart timer.
    curr_ack = abs_ackno;

    con_retran = 0;
    Rto = _initial_retransmission_timeout;
    rTimer.stop();
    if (fbytes != 0)
        rTimer.start(Rto);
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    rTimer.elapsed(ms_since_last_tick);
    if (!rTimer.is_run()) {
        if (buffer.empty())  // if buffer empty, we can't retransmission segment
            return;
        _segments_out.push(buffer[0].second);
        con_retran += 1;
        if (is_detective)
            ;
        else
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

void TCPSender::send_syn() {
    is_send_syn = true;
    TCPSegment t;
    send_segments(t, 1, true, false);
    if (!rTimer.is_run() && fbytes != 0)
        rTimer.start(Rto);
}

void TCPSender::send_segments(TCPSegment &t, uint64_t len, bool is_syn, bool is_fin) {
    if (is_syn)
        t.header().syn = true;
    if (is_fin)
        t.header().fin = true;
    t.header().seqno = next_seqno();
    t.payload() = Buffer(_stream.read((is_syn || is_fin) ? len - 1 : len));
    _segments_out.push(t);                        // send to peer
    buffer.push_back(make_pair(_next_seqno, t));  // store for retransmission
    wSize = wSize > len ? wSize - len : 0;
    fbytes += len;
    _next_seqno += len;
}
