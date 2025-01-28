#define _SMM_SAMPLER_RINGBUF_SHOW_PRODUCER_FUNCTIONS_
#include "sampler_ringbuf.h"

static samplerd_blocks_t blocks;

void sampler_ringbuf_restart(uint32_t num_of_channels) {
    blocks.physical_byte = 0;
    blocks.logical_block = 0;
    blocks.num_of_channels = num_of_channels;
    blocks.block_pool[blocks.logical_block].first_sample_index = 0;
}

void sampler_ringbuf_push_sample(uint8_t *value12bit_l_aligned) {
    if (blocks.physical_byte >= NEW_SAMPLER__BLOCK_BYTE_CAPACITY) {
        uint32_t last_sample_index = blocks.block_pool[blocks.logical_block % NEW_SAMPLER__BLOCK_COUNT].first_sample_index;
        blocks.logical_block++;
        blocks.physical_byte = 0;
        blocks.block_pool[blocks.logical_block % NEW_SAMPLER__BLOCK_COUNT].first_sample_index =
            last_sample_index + (NEW_SAMPLER__BLOCK_SAMPLE_CAPACITY) / blocks.num_of_channels;
    }
    uint8_t *buffer = blocks.block_pool[blocks.logical_block % NEW_SAMPLER__BLOCK_COUNT].buffer;
    if (blocks.physical_byte % 3 == 0) {
        buffer[blocks.physical_byte++] = value12bit_l_aligned[0];
        buffer[blocks.physical_byte] = value12bit_l_aligned[1] & 0xF0;
    } else {
        buffer[blocks.physical_byte++] |= value12bit_l_aligned[0] >> 4;
        buffer[blocks.physical_byte++] = (value12bit_l_aligned[0] << 4) | (value12bit_l_aligned[1] >> 4);
    }
}

inline const samplerd_sample_block_t *sampler_ringbuf_access_block_unchecked(uint32_t logical_block) {
    return &blocks.block_pool[logical_block % NEW_SAMPLER__BLOCK_COUNT];
}

inline uint32_t sampler_ringbuf_get_current_logical_block_unchecked() {
    return blocks.logical_block;
}
