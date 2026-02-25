#### uImage编译说明

1. rootfs准备

   - [ ] 使用root权限将rootfs_pub.tgz解压到board_support/rootfs目录下

2. 将build_uImage.sh文件放在SDK根目录下

3. 执行build_uImage.sh，生成的uImage.img在board_support/tools/flash_upgrade目录下

   `./build_uImage.sh`



#### 工厂量产版本Flash.img编译方法

1. 将MC632X_linux_fast.ini和custom_header.bin放置在board_support/tools/flash_upgrade目录下

2. 通过命令切换到board_support/tools/flash_upgrade目录，执行

   `./mkflashimgv4 MC632X_linux_fast.ini`



