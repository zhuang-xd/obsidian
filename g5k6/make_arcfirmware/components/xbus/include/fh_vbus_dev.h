#ifndef __fh_vbus_dev_h__
#define __fh_vbus_dev_h__

struct _fh_vbus_dev_
{
    struct rt_device rtdev;
    rt_size_t (*seq_read)(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size);
    rt_size_t (*seq_write)(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size);
};

typedef struct _fh_vbus_dev_ fh_vbus_dev_t;

rt_err_t rt_vbus_register_service(const char *sname, unsigned int priority, int is_server);
int rt_vbus_device_register(fh_vbus_dev_t *vbusdev, const char *devname, const char *servname, rt_uint32_t flag);
int rt_vbus_device_register_ex(fh_vbus_dev_t *vbusdev, const char *serv_name, int priority, rt_uint32_t flag);

#endif /*__fh_vbus_dev_h__*/

