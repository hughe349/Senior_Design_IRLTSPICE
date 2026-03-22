#include "core/board_info.hpp"

ColConIter ChainedCrossbar::cols_begin() const { return ColConIter(this->bars); }

ColConIter::ColConIter(std::vector<PhysCrossbar> const &bars) {
    this->bars = &bars;
    this->bar_id = 0;
    this->col_id = 0;
}

ColConIter::ColConIter() { this->bars = nullptr; }

ColCon ColConIter::operator*() { return (*bars)[bar_id].cols[col_id]; }

ColCon const *ColConIter::operator->() { return &(*bars)[bar_id].cols[col_id]; }

// Prefix
ColConIter &ColConIter::operator++() {

    // Don't incr the too-far
    if (bars != nullptr) {
        col_id++;
        // Use a while in case we get a fucked crossbar with zero pins or something
        while (col_id >= (*bars)[bar_id].cols.size()) {
            bar_id++;
            if (bar_id >= bars->size()) {
                bars = nullptr;
                return *this;
            }
            col_id = 0;
        }
    }

    return *this;
}

// Postfix
ColConIter ColConIter::operator++(int) {
    ColConIter temp = *this;

    ++*this;

    return temp;
}

bool ColConIter::operator==(ColConIter const &other) const {
    if (bars == other.bars) {
        if (bars == nullptr) {
            return true;
        } else {
            return bar_id == other.bar_id && col_id == other.col_id;
        }
    } else {
        return false;
    }
}
bool ColConIter::operator!=(ColConIter const &other) const { return !(*this == other); }

uint8_t ColConIter::get_bar_phys_id() { return (*bars)[bar_id].id; }
uint8_t ColConIter::get_bar_col() { return col_id; }
