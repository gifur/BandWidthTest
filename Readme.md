The project is used to test the tcp transfer speed between client side with server side.

First, you should launch the **BandWidthServer.exe** at server side to listern client's connection. and then run the **BandWidthClient.exe** with desired options.

you can upload[/downlod] fixed amount of data to[/from] server and then calculate the time waste, or provide certain amount of time and see how many data the client upload[/downlod] to[/from] server.

Finally, figurate out and show the speed statistics.

**服务端 BandWidthServer.exe**

* 参数说明（usage）：
  * **-p <*port*>**: indicate the port to listern

**客户端 BandWidthClient.exe**

* 参数说明（usage）：
  * **-u**: test the uplod speed
  * **-d**: test the downlod speed
  * **-c <*server_ip*>**: indicate the server ip to connect who run the **BandWidthService.exe**, if not set, point to local address by default.
  * **-p <*port*>**：indicate the server port to connect, set 5150 by default
  * **-i <*interval_second*>**: display the speed statistic in interval second
  * **-t <*total_test_time*>**: indicate the time you will use to test, the unit is second, set 10s by default. **If set this, do not use *-n* option !**
  * **-n <*total_test_data_size*>[K|M|k|m]**: K|M(upper char) indicates **Byte** type while k|m(lower char) indicates **bit** type
  * **-h**: display the help information about param usage

**Snapshot**

![image-01](http://oxutubpgi.bkt.clouddn.com/18-11-23/59216321.jpg)
