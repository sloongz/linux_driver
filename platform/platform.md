- platform

Linux设备模型是设备、驱动、总线模型，一些不存在实际总线， 可以通过CPU

bus寻址的设备可以套用这个模型。

测试

```
插入
# insmod platform.ko

查看总线上注册的设备和驱动
$ ls /sys/bus/platform/devices/x_dev
driver  driver_override  modalias  power  subsystem  uevent

$ ls /sys/bus/platform/drivers/x_dev/
bind  module  uevent  unbind  x_dev


删除
# rmmod platform 

# dmesg
[10749.567739] x_drv_probe
[10749.567758] work_func, cnt:0
[10749.569349] x_init end
[10750.585331] work_func, cnt:1
[10751.609504] work_func, cnt:2
[10752.633545] work_func, cnt:3
[10753.657594] work_func, cnt:4
[10754.681622] work_func, cnt:5
[10755.705676] work_func, cnt:6
[10756.729638] work_func, cnt:7
[10757.752777] x_drv_remove end
[10757.752900] x_exit end
```

注册了设备和驱动， 名字匹配后调用probe，在probe中起了个任务，

然后在删除模块的时候注销设备和驱动。