#ifndef __HOST_DEVICE_MATRIX_H__
#define __HOST_DEVICE_MATRIX_H__

struct t_HostDeviceMatrix {
  int width;
  int height;
  double *host;
  double *device;
};
typedef struct t_HostDeviceMatrix DataMatrix;
typedef struct t_HostDeviceMatrix ResultsMatrix;

#endif // __HOST_DEVICE_MATRIX_H__