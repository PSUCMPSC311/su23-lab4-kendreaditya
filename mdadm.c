#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mdadm.h"
#include "jbod.h"



int mdadm_mount(void) {
  return -1;
}

int mdadm_unmount(void) {
  return -1;
}

int mdadm_read(uint32_t start_addr, uint32_t read_len, uint8_t *read_buf)  {
  	return read_len;
}


int mdadm_write(uint32_t start_addr, uint32_t write_len, const uint8_t *write_buf) {
  	return write_len;
}
