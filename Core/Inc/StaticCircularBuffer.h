/*
 * StaticCircularBuffer.h
 *
 *  Created on: Oct 25, 2025
 *      Author: matve
 */

#ifndef INC_STATICCIRCULARBUFFER_H_
#define INC_STATICCIRCULARBUFFER_H_


template<typename T>
class StaticCircularBuffer {
private:
    T buffer[64];
    size_t head;
    size_t tail;
    size_t item_count;
    uint8_t sup;

public:
    StaticCircularBuffer() : head(0), tail(0), item_count(0) {}

    bool init(uint8_t N) {
    	this->sup = N;
    }

    // Add item
    bool push(const T& item) {
        if (full()) {
            return false;
        }

        buffer[head] = item;
        head = (head + 1) % sup;
        item_count++;
        return true;
    }

    // Add item with overwrite
    void push_overwrite(const T& item) {
        buffer[head] = item;
        head = (head + 1) % sup;

        if (full()) {
            tail = (tail + 1) % sup;
        } else {
            item_count++;
        }
    }

    // Extraction item
    bool pop(T& item) {
        if (empty()) {
            return false;
        }

        item = buffer[tail];
        tail = (tail + 1) % sup;
        item_count--;
        return true;
    }

    // View item
    bool peek(T& item) const {
        if (empty()) {
            return false;
        }

        item = buffer[tail];
        return true;
    }

    // View item by index (0 - next)
    bool peek_at(size_t index, T* item) const { //TODO: check it
        if (index >= item_count) {
            return false;
        }

        *item = buffer[(tail + index) % sup];
        return true;
    }

    bool contains(T item) {
    	for (int i = 0; i < sup; i++) {

    	}
    }

    bool empty() const { return item_count == 0; }
    bool full() const { return item_count == sup; }
    size_t size() const { return item_count; }
    constexpr size_t capacity() const { return sup; }

    // Use it, if you're *&&*^& idiot
    const T* data() const { return buffer; }

    void clear() { head = tail = item_count = 0; }
};


#endif /* INC_STATICCIRCULARBUFFER_H_ */
