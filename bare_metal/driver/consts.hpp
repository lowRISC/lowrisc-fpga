#ifndef LoRCCoreplex_HD
  #define LoRCCoreplex_HD
  #define ADD_BRAM
  #define ADD_DUMH
  #define ADD_ROCKET_INT
  #define ADD_SPI
  #define ADD_UART
  #define BRAM_BASE (0x40000000ull)
  #define BRAM_SIZE (0x100000ull)
  #define DUMH_BASE (0x41000000ull)
  #define DUMH_SIZE (0x1000ull)
  #define INT_SPI_BASE (0x00000001u)
  #define INT_SPI_SIZE (0x00000001u)
  #define INT_UART_BASE (0x00000000u)
  #define INT_UART_SIZE (0x00000001u)
  #define MEM_ADDR_WIDTH (0x00000020u)
  #define MEM_BASE (0x0000000080000000u)
  #define MEM_DATA_WIDTH (0x00000040u)
  #define MEM_ID_WIDTH (0x00000004u)
  #define MEM_SIZE (0x0000000010000000u)
  #define MMIO_MASTER_ADDR_WIDTH (0x0000001fu)
  #define MMIO_MASTER_DATA_WIDTH (0x00000040u)
  #define MMIO_MASTER_ID_WIDTH (0x00000008u)
  #define MMIO_SLAVE_ADDR_WIDTH (0x00000020u)
  #define MMIO_SLAVE_DATA_WIDTH (0x00000040u)
  #define MMIO_SLAVE_ID_WIDTH (0x00000008u)
  #define ROCKET_INT_SIZE (0x00000002u)
  #define SPI_BASE (0x41004000ull)
  #define SPI_SIZE (0x2000ull)
  #define UART_BASE (0x41002000ull)
  #define UART_SIZE (0x2000ull)
#endif
