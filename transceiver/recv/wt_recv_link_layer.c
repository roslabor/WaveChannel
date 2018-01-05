#include "wt_recv_link_layer.h"
#include "proto_utils/wt_proto_link_layer.h"
#include "wt_recv_physical_layer.h"
#include "proto_utils/wt_proto_common.h"

#include <stdlib.h>
#include <string.h>

static WaveTransPackageHalf package_temp_;
static int package_temp_addr_ = 0;

static WaveTransPackageMix mux_package_temp_;
static int package_mux_addr_ = 0;


static int GetNextPackage(WaveTransPackage *package)
{
  int ret;
  if (package_temp_addr_ >= 0 && package_temp_addr_ < START_FREQ_NUM) {    //find package start mark
re_read_st: while((ret = WTRecvPhyLayerGetData(&package_temp_.st_mark_[package_temp_addr_], START_FREQ_NUM - package_temp_addr_))>0){
       int i;
       for (i = 0; i < ret; i++) {
         if (WTLinkCheckStMark(package_temp_.st_mark_[package_temp_addr_ + i], package_temp_addr_ + i) != 1) {
           package_temp_addr_ = 0;
           goto re_read_st;
         }
       }
       package_temp_addr_ += ret;
       break;
    }
  }
  if (package_temp_addr_ >= START_FREQ_NUM&&package_temp_addr_ < START_FREQ_NUM + HBYTE_DATA_NUM) {     //read half_byte_data_
    int dst_addr = START_FREQ_NUM + HBYTE_DATA_NUM;
    ret = WTRecvPhyLayerGetData(&package_temp_.half_byte_data_[package_temp_addr_ - START_FREQ_NUM], dst_addr - package_temp_addr_);
    if (ret <= 0) {
      return -1;
    }
    package_temp_addr_ += ret;
  }
  /*read crc_half_byte_data_*/
  if (package_temp_addr_ >= (START_FREQ_NUM + HBYTE_DATA_NUM) && package_temp_addr_ < (START_FREQ_NUM + HBYTE_DATA_NUM + HBYTE_CHECKSUM_NUM)) {
    int dst_addr = START_FREQ_NUM + HBYTE_DATA_NUM + HBYTE_CHECKSUM_NUM;
    int addr_temp = package_temp_addr_ - (START_FREQ_NUM + HBYTE_DATA_NUM);
    ret = WTRecvPhyLayerGetData(&package_temp_.check_half_byte_data_[addr_temp], dst_addr - package_temp_addr_);
    if (ret <= 0) {
      return -1;
    }
    package_temp_addr_ += ret;
  }
  /*check one package*/
  if (package_temp_addr_ == START_FREQ_NUM + HBYTE_DATA_NUM + HBYTE_CHECKSUM_NUM) {
    WaveTransPackage temp;
    WTLinkHalfPackageToByte(&package_temp_, &temp);
    if (WTLinkChecksumOk(&temp) != 1) {
      package_temp_addr_ = 0;
      return -1;
    }
    memcpy(package, &temp, sizeof(WaveTransPackage));
    package_temp_addr_ = 0;
    return 0;
  }
  return -1;
}

static int GetNextPackageMux(WaveTransPackageMux *package)
{
  int ret;
  if (package_mux_addr_ >= 0 && package_mux_addr_ < MIXING_BYTE_ST_NUM) {    //find package start mark
  re_read_st: while ((ret = WTRecvPhuLayerGetDataForMixing(&mux_package_temp_.st_mark_[package_mux_addr_], MIXING_BYTE_ST_NUM - package_mux_addr_))>0) {
    int i;
    for (i = 0; i < ret; i++) {
      if (WTLinkCheckStMarkMux(&mux_package_temp_.st_mark_[package_mux_addr_ + i], package_mux_addr_ + i) != 1) {
        package_mux_addr_ = 0;
        goto re_read_st;
      }
    }
    package_mux_addr_ += ret;
    break;
  }
  }
  if (package_mux_addr_ >= MIXING_BYTE_ST_NUM&&package_mux_addr_ < MIXING_BYTE_ST_NUM + MIXING_BYTE_DATA_NUM) {     //read half_byte_data_
    int dst_addr = MIXING_BYTE_ST_NUM + MIXING_BYTE_DATA_NUM;
    ret = WTRecvPhuLayerGetDataForMixing(&mux_package_temp_.byte_data_[package_mux_addr_ - MIXING_BYTE_ST_NUM], dst_addr - package_mux_addr_);
    if (ret <= 0) {
      return -1;
    }
    package_mux_addr_ += ret;
  }
  /*read crc_half_byte_data_*/
  if (package_mux_addr_ >= (MIXING_BYTE_ST_NUM + MIXING_BYTE_DATA_NUM) && package_mux_addr_ < (MIXING_BYTE_ST_NUM + MIXING_BYTE_DATA_NUM + MIXING_CHECKSUM_NUM)) {
    int dst_addr = MIXING_BYTE_ST_NUM + MIXING_BYTE_DATA_NUM + MIXING_CHECKSUM_NUM;
    int addr_temp = package_mux_addr_ - (MIXING_BYTE_ST_NUM + MIXING_BYTE_DATA_NUM);
    ret = WTRecvPhuLayerGetDataForMixing(&mux_package_temp_.check_byte_data_[addr_temp], dst_addr - package_mux_addr_);
    if (ret <= 0) {
      return -1;
    }
    package_mux_addr_ += ret;
  }
  /*check one package*/
  if (package_mux_addr_ == MIXING_BYTE_ST_NUM + MIXING_BYTE_DATA_NUM + MIXING_CHECKSUM_NUM) {
    WaveTransPackageMux temp;
    WTLinkMuxPackageToByte(&mux_package_temp_, &temp);
    if (WTLinkChecksumOkMux(&temp) != 1) {
      package_mux_addr_ = 0;
      return -1;
    }
    memcpy(package, &temp, sizeof(WaveTransPackageMux));
    package_mux_addr_ = 0;
    return 0;
  }
  return -1;
}

int WTRecvLinkLayerInit(void)
{
  package_temp_addr_ = 0;
  return 0; 
}

void WTRecvLinkLayerExit(void)
{
  package_temp_addr_ = 0;
}

int WTRecvLinkLayerGetData(void * buf, int buf_len)
{
  WaveTransPackage one_package;
  int buf_w_addr = 0;
  while (buf_len - buf_w_addr >= HBYTE_DATA_NUM / 2) {
    if (GetNextPackage(&one_package) != 0) {
      break;
    }
    memcpy(((unsigned char *)buf + buf_w_addr), one_package.byte_data_, sizeof(unsigned char)*one_package.real_data_num_);
    buf_w_addr += one_package.real_data_num_;
  }
  return buf_w_addr;
}

int WTRecvLinkLayerInitForMix(void)
{
  package_mux_addr_ = 0;
  return 0;
}

void WTRecvLinkLayerExitForMix(void)
{
  package_mux_addr_ = 0;
}

int WTRecvLinkLayerGetDataForMix(void * buf, int buf_len)
{
  WaveTransPackageMux one_package;
  int buf_w_addr = 0;
  while (buf_len - buf_w_addr >= MIXING_BYTE_DATA_NUM) {
    if (GetNextPackageMux(&one_package) != 0) {
      break;
    }
    memcpy(((unsigned char *)buf + buf_w_addr), one_package.byte_data_, sizeof(unsigned char)*one_package.real_data_num_);
    buf_w_addr += one_package.real_data_num_;
  }
  return buf_w_addr;
}
