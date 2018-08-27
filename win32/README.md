# win32版本使用说明

 1. include文件夹包含需要引入的SDK头文件；
 2. target文件夹包含SDK编译后的库文件，分为Debug、Release两个版本；
 3. pandora_c_sdk文件夹包含一个sample工程，引用了1、2中的头文件和库文件，可参考该示例工程将SDK引入到您自己的工程中。
 
> 注意：发布程序时需要带上target\Release文件夹下的所有依赖库。
