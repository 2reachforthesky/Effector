#ifndef _config_hpp
#define _config_hpp

#define SAMPLE_RATE		48000	// Hz
#define BLOCK_SIZE		128	// samples per processing block
#define QUEUE_SIZE_MSECS		1000	// size of the TX/RX queues in milliseconds

#define QUEUE_BLOCKS		4	// size of the TX/RX queues in blocks
#define PREFILL_BLOCKS		2	// silence written before start
#define OUTPUT_PREFILL_BLOCKS    4	// silence written before start
#define OUTPUT_MAX_QUEUED_FRAMES	512	// outq がこれを超えたら1サンプル捨てる

#define INPUT_LEFT_CHANNEL	true	// false: use right channel of AG03

#endif