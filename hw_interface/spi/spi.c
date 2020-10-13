static struct spi_board_info spi_info_xxx[] = { 
	{   
		.modalias = "spi_xxx",  /* 对应的spi_driver名字也是"oled" */
		.max_speed_hz = 1000000,      /* max spi clock (SCK) speed in HZ */
		.bus_num = 1,
		.mode    = SPI_MODE_0,
		//.irq = XXX_IRQ,
		.platform_data = &xxx_platform_data,
		.chip_select = 0,
	},  
};

static int xxx_spi_probe(struct spi_device *func)
{
	return 0;
}

static int xxx_spi_disconnect(struct spi_device *func)
{
	printk(KERN_INFO "%s start\n", __func__);
	return 0;	
}

static struct spi_driver spi_driver_xxx = { 
	.probe      = xxx_spi_probe,
	.remove     = xxx_spi_disconnect,
	.driver = { 
		.name       = "spi_xxx",
		.bus            = &spi_bus_type,
		.owner          = THIS_MODULE,
		//.of_match_table = of_match_ptr(xxx);
		//#ifdef CONFIG_PM
		//		.pm     = &xxx_pm_ops,
		//#endif
	},  
};

#ifdef MODULE
static void device_spi_delete(struct spi_master *master, unsigned int cs)
{
	struct device *dev;
	char str[32];

	snprintf(str, sizeof(str), "%s.%u", dev_name(&master->dev), cs);

	dev = bus_find_device_by_name(&spi_bus_type, NULL, str);
	if (dev) {
		printk(KERN_INFO "Deleting %s\n", str);
		device_del(dev);
	}
}

static int device_spi_device_register(struct spi_board_info *spi)
{
	struct spi_master *master;

	master = spi_busnum_to_master(spi->bus_num);
	if (!master) {
		pr_err("spi_busnum_to_master(%d) returned NULL\n",
					spi->bus_num);
		return -EINVAL;
	}
	/* make sure it's available */
	device_spi_delete(master, spi->chip_select);
	spi_device = spi_new_device(master, spi);
	put_device(&master->dev);
	if (!spi_device) {
		dev_err(&master->dev, "spi_new_device() returned NULL\n");
		return -EPERM;
	}
	return 0;
}
#else
static int device_spi_device_register(struct spi_board_info *spi)
{
	return spi_register_board_info(spi, ARRAY_SIZE(spi_info_xxx));
}
#endif

static int __init hello_init(void)
{
	printk(KERN_INFO "hello world enter\n");
	// register spi device
	device_spi_device_register(spi_info_xxx);

	// register spi driver
	spi_register_driver(&spi_driver_xxx);

	return 0;
}
