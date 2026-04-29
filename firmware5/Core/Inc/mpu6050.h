#ifndef __MPU6050_H__
#define __MPU6050_H__

#include "main.h"

#define MPU6050_ADDR     (0x68 << 1)   // 7-bit address 0x68 → HAL uses 8-bit

// MPU6050 Register Addresses
#define MPU6050_RA_PWR_MGMT_1      0x6B
#define MPU6050_RA_SMPLRT_DIV      0x19
#define MPU6050_RA_CONFIG          0x1A
#define MPU6050_RA_GYRO_CONFIG     0x1B
#define MPU6050_RA_ACCEL_CONFIG    0x1C
#define MPU6050_RA_ACCEL_XOUT_H    0x3B
#define MPU6050_RA_WHO_AM_I        0x75

#define MPU6050_RA_GYRO_XOUT_H     0x43
#define MPU6050_RA_GYRO_XOUT_L     0x44
#define MPU6050_RA_GYRO_YOUT_H     0x45
#define MPU6050_RA_GYRO_YOUT_L     0x46
#define MPU6050_RA_GYRO_ZOUT_H     0x47
#define MPU6050_RA_GYRO_ZOUT_L     0x48

// Function prototypes
uint8_t MPU6050_Init(void);

void MPU6050_Read_Accel(float *ax, float *ay, float *az);
void MPU6050_Read_Gyro(float *gx, float *gy, float *gz);


#endif
