
#define STM_TRACE(id, value) \
  { \
    asm volatile ("mv   a0,%0": :"r" ((uint64_t)value) : "a0");	\
    asm volatile ("csrw 0x8ff, %0" :: "r"(id));	\
  }

static inline void trace_event0() {
  STM_TRACE(0, 0x42);
}

static inline void trace_event1(uint64_t value) {
  STM_TRACE(1, value);
}

static inline void trace_event2(uint64_t id, uint64_t value) {
  STM_TRACE(id, value);
}

