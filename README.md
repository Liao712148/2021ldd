2021 LDD3 第2章
===

contribute by `Liao712148`
###### tags: `Linux Device Drivers`, `Linux `


### 遇到的問題
**ERROR: could not insert module hello.ko: Operation not permitted**
解決辦法：
根據
[insmod: ERROR: could not insert module HelloWorld.ko: Operation not permitted](https://stackoverflow.com/questions/58546126/insmod-error-could-not-insert-module-helloworld-ko-operation-not-permitted)
會使用到
[Why do I get “Required key not available” when install 3rd party kernel modules or after a kernel upgrade?](https://askubuntu.com/questions/762254/why-do-i-get-required-key-not-available-when-install-3rd-party-kernel-modules)

根據
>sudo apt install mokutil
sudo mokutil --disable-validation

之後reboot 

---
Makefile 是一種腳本，用來生成模組，
## Makefile

```cpp=

# To build modules outside of the kernel tree, we run "make"
# in the kernel source tree; the Makefile these then includes this
# Makefile once again.
# This conditional selects whether we are being included from the
# kernel Makefile or not.

ifneq ($(KERNELRELEASE),)
# call from kernel build system

obj-m	:= hello.o

else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif



clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.mod modules.order *.symvers *.dwo *.mod.dwo

```

*    第8行：我們寫的Makefile第一次被執行，此時KERNELRELEASE還沒被定義，因此條件不成立。執行else的內容。
*    第15行：因為`?=`表示如果左邊的變數上未定義，就將右邊賦值給左邊。否則維持原來的值。而`/lib/modules/$(shell uname -r)/build`是核心原碼樹的頂層目錄。![](https://i.imgur.com/8j6CmNn.png)，會有一個Makefile幫助我們完成模組。
*    第16行：PWD變數紀錄當的Makefile所在的位子。
*    第18,19行：一開始會切換目錄到`-C`選項所指的目錄（核心原碼樹的頂層目錄），在該處找出核心原碼樹頂層目錄的Makefile。而`M=`選項會使得該Makefile回到模組的原始程式目錄，然後才開始建構第18行所定義的`modules`目標，此目標代表`obj-m`變數所列的模組，也就是`hello.o`。執行第19行時，會造成我們所寫的Makefile又再被執行了一次，此次條件成立，因此會執行第11行。
*    第11行：它宣稱有一個模組要從目的碼`hello.o`建構出來，所產生的模組檔是`hello.ko`。與傳統的Makefile不同，使用了GNU make的擴充語法，樣讓這個Makefile有作用，必須從核心建構系統來調用它 ==> ==利用核心的Makefile來產生模組==。
*    第25行：清除檔案。



```cpp=
#include <linux/init.h>//定義了模組所需的許多函式與變數
#include <linux/module.h>//模組的初始函式與清理函式
MODULE_LICENSE("Dual BSD/GPL");
static int hello_init(void) {
    printk(KERN_ALERT "Hello, world\n");
    return 0;
}
static void hello_exit(void) {
    printk(KERN_ALERT "Goodbye, cruel world\n");
}
module_init(hello_init);
module_exit(hello_exit);
```
*    第1,2行：定義於`/usr/src/linux-headers-5.11.0-27-generic/include/linux`。
*    第3行：用來讓kernel知道此模組遵守自由授權條款。
*    第11行：在模組載入kernel時會執行module_init巨集中的函式。也就是helo_init。初始函式是向核心註冊模組所提供新功能（可能是整個驅動程式，或是可供應用程式存取的軟體抽象層）。可以將此函式改成`static int __int hello_init()`當中的`__init`讓核心可以知道該函式==只用於初始期間==。
*    第12行：在模組卸載時會執行module_exit巨集中的函式。也就是helo_exit。清理函式負責在模組被卸載之前，註銷曾這冊的軟體介面，並將資源還給系統。以將此函式改成`static void __exit hello_exit()`當中的`__exit`讓核心可以知道該函式==只用於卸載期間==。
*    第5行：使用的是`printk`而不是`printf`，因為`printf`屬於libc，==模組不能與一般函式庫連結，只能與核心連結==，因此要使用定義於linux核心內的`printk`，可供模組使用。而當中的`KERN_ALERT`代表訊息的優先度，我們刻意指定一個高的優先度，因為預設的優先度不一定會使得訊息出現在你看得到的地方。
---



```cpp=
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>//只要引入sched.h就可以將current當成當時執行的process。
#include <linux/moduleparam.h>
MODULE_LICENSE("Dual BDE/GPL");
static char *whom = "world";
static int howmany = 1;
module_param(howmany, int, S_IRUGO);
module_param(whom, charp, S_IRUGO);
static int hello_init(void) {
    printk(KERN_ALERT "Hello world\n");
    printk(KERN_ALERT "The init process is \"%s\" (pid %i)\n", current->comm, current->pid);
    int i;
    for(i = 0; i < howmany; i++) {
	    printk(KERN_ALERT "%s %d times\n", whom, howmany);
    }
    return 0;
}
static void hello_exit(void) {
    printk(KERN_ALERT "Goodbye, curel world\n");
    printk(KERN_ALERT "The exit process is \"%s\" (pid %i)\n", current->comm, current->pid);
}

module_init(hello_init);
module_exit(hello_exit);

```
*    第3, 11, 21行：核心的大部份動作，都是在代替當時正在執行的process執行遠本無法在user-space完成的工作。透過定義於`<asm/current.h>`的全局變數`current`，current永遠指向當時正在執行的process。透過current，核心程式可以取得當時正在執行的狀態資訊。
*    第6，7，8，9行：在`insmod`時改變系統的參數，模組本身要先以`module_param`巨集來宣告參數。

>`#` insmod hello.ko howmany=10 whom="Mom"

==所有的模組參數都必須有預設值==，`module_param()`的最後一欄是權限值，決定模組參數在sysfs裡的對應項目的存取權限。



### 參考資料
[Makefile 語法和示範](https://hackmd.io/@sysprog/SySTMXPvl)
[編譯內核模塊的的Makefile的講解](https://www.twblogs.net/a/5b81bc442b71772165ae0321)
[驅動模組Makefile解析](https://www.itread01.com/p/1377069.html)

2021 LDD3 第3章
===

contribute by `Liao712148`
###### tags: `Linux Device Drivers`, `Linux `

寫驅動程式的第一步，是==定義你的驅動程式對user-space的程式提供那些功能==。


#### 名詞解釋
*    裝置編號
    *    主編號:代表裝置(指scull從kernel配置而來的記憶體)所配合的驅動程式。
         可以透過由<linux/kdev_t.h>當中的macro `MAJOR(det_t dev);`來得到。
    *    次編號:代表你的驅動程式所實作的裝置，驅動程式可以直接從核心取得一個指向目標裝置的指標。
        可以透過由<linux/kdev_t.h>當中的macro `MINOR(det_t dev);`來得到。
    *   裝置編號可以利用 `MKDEV(int major, int minor)`來得到。

#### 向kernel索取裝置編號
`int alloc_chrdev_region(dev_t *dev, unsigned int firstmintor,   unsigned int count, char *name);`
透過`dev`取得配置範圍的第一個裝置編號。

#### /proc/devices和/dev的關係
`/proc/devices/`中的裝置是通過insmod載入到核心的，它可產生一個major供mknod作為 引數。
`/dev/* `是通過mknod加上去的，格式:mknod device1 c/b major minor 如：mknod /dev/ttyS0 c 4 64，使用者通過此裝置名來訪問你的驅動。

#### 透過script 來載入scull module
透過scull_load來載入module
```shell=
#!/bin/sh
module="scull"
device="scull"
mode="664"
# Group: since distributions do it differently, look for wheel or use staff
if grep -q '^staff:' /etc/group; then
    group="staff"
else
    group="wheel"
fi

# invoke insmod with all arguments we got
# and use a pathname, as insmod doesn't look in . by default
insmod ./$module.ko $* || exit 1

# retrieve major number
major=$(awk "\$2==\"$module\" {print \$1}" /proc/devices)

# Remove stale nodes and replace them, then give gid and perms
# Usually the script is shorter, it's scull that has several devices in it.

rm -f /dev/${device}[0-3]
mknod /dev/${device}0 c $major 0
mknod /dev/${device}1 c $major 1
mknod /dev/${device}2 c $major 2
mknod /dev/${device}3 c $major 3
ln -sf ${device}0 /dev/${device}
chgrp $group /dev/${device}[0-3]
chmod $mode  /dev/${device}[0-3]
```
`第5行`:664表示只有owner,group才有寫入的權限，others只有讀取的權限。[檔案權限](https://shian420.pixnet.net/blog/post/344938711-%5Blinux%5D-chmod-%E6%AA%94%E6%A1%88%E6%AC%8A%E9%99%90%E5%A4%A7%E7%B5%B1%E6%95%B4!)

`第15行`:載入module`insomd ./$module.ko`，因為insmod不會尋找當時工作目錄所以模組檔名之前要加上`./`，並利用`$*`傳入所有命令列的所有引數。最後有一個`||`的操作，如果載入不成功就會exit。

`第18行`:[awk類似grep](https://ithelp.ithome.com.tw/articles/10136126)， /proc/devices的第一欄($1)事裝置的主編號。第二攔($2)是裝置名稱。

`第23行`:移除前次裝載所留下來的節點。

`第7~11行`:設定節點的擁有者與群組，有些系統使用wheel群組，但有些系統使用staff群組，所以需要用`if grep -q '^staff:' /etc/group`來判斷。其中`grep -q '^staff:`的`^staff:`代表只搜尋行首的字元，staff:是要搜尋的字元,並且是要在該行的行首。

`第29行`:利用[chgrp](https://www.itread01.com/content/1523008588.html)，更改`/dev/${device}[0-3]`的屬主，改成`$group`。讓`$group`的成員也有權寫入scull裝置。

`第30行`:利用[chmod](https://www.itread01.com/content/1523008588.html)，更改`/dev/${device}[0-3]`的九個屬性，改成`$mode`代表只有owner,group才有寫入的權限，others只有讀取的權限。
[如號撰寫shell script](https://blog.techbridge.cc/2019/11/15/linux-shell-script-tutorial/)
#### 取得裝置編號
==當驅動程式設定字元裝置時，要做的第一件事事先取得一個或多個裝置編號==
```cpp=
int scull_init_module(void)
{
	int result, i;
	dev_t dev = 0;

	/*
	 * Get a range of minor numbers to work with, asking for a dynamic
	 * major unless directed otherwise at load time.
	 */
	if (scull_major) {
		dev = MKDEV(scull_major, scull_minor);
		/*使用靜態分配，所以必須事先知道major*/
        result = register_chrdev_region(dev, scull_nr_devs, "scull");
	} else {
        /*使用動態分配*/
		result = alloc_chrdev_region(&dev, scull_minor, scull_nr_devs,
				"scull");
		scull_major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "scull: can't get major %d\n", scull_major);
		return result;
	}
    /*...*/
}
```



#### 重要的資料結構
驅動程式中的基本操作，多半涉及kernel的3種重要的資料結構，分別是`file_operations`,`file`,`inode`。

##### file_operatoins
定義在`<linux/fs.h>`，是一個操作檔案方法的集合。成員多是function pointer，所以file_operations這個資料結構有點像是工具箱，每個工頭(file)的工具相當中的工具部會完全相同，透過這些工具箱的內容，我們就可以知道這個工頭會哪一些東西。對於每個已經開啟的檔案（裝置）會以file結構表示，核心會各自賦與ㄧ專屬的作業函式，也就是file結構中的f_op會指向該檔案的作業函式的集合file_operations結構。因此在對該檔案（裝置）呼叫system call時，會有專屬於該檔案（裝置）作業函式來對接。

##### file
定義在`<linux/fs.h>`的struct file，==file與user space當中的FILE指標毫無關西==，FILE是定義於C函式庫，不可能出現在kernel當中的程式碼中，而file結構也不會出現在user space的應用程式裡。
file結構代表已開啟的檔案(open file)，這不是驅動程式的專利，對於系統上每個已經開啟的檔案，在kernel space都有一個對應的struct file。每一個file結構都是kernel在收到open()系統呼叫時自動成立的，然後在最後一次close()系統呼叫返回。也就是沒有任何process持有file結構的指標時，kernel才會釋放file結構。
file struct成員當中最重要的就是 `struct file_operations *f_op`，他指向了上述的file_operations。

##### inode
Kernel內部使用inode結構來代表檔案，它不同於file結構，file結構是代表以開啟的檔案的FD(file descriptor)。如果同一個檔案配重複開啟多次，這樣的話kernel會有多個代表FD的file結構，但是他們==都指向同一個inode結構==。indoe結構中我們只在意
*    dev_t i_rdev
對於代表裝置檔的inodes，此欄位含有實際的裝置編號。
    ==盡量使用以下macro，避免直接操作i_rdev==
    *`unsigned int iminor(struct inode *inode);`
    *`unsigned int imajor(struct inode *inode);`
*    struct cdev *i_cdev
kernel內部用來表示字元裝置的結構，對於代表字元裝置的inode，此欄位含有一個指向該結構的指標。

#### 註冊字元裝置
##### struct cdev
是裝置與核心之間的介面。由於我們希望將struct cdev(linux/cdev.h)這個結構
![](https://i.imgur.com/FOEW25z.png)
嵌入到我們所定義的結構（scull_dev)之中，
![](https://i.imgur.com/eVxm9TN.png)
因此要使用`cdev_add`初始化cdev的結構，並將cdev結構中的`*ops`指向我們所定義的file_operations(scull_fops)。


#### 載入scull模組

此時的模組
![](https://i.imgur.com/UlxGleY.png)

/proc/devices 的內容
![](https://i.imgur.com/RDunuaB.png)

/dve 的內容
![](https://i.imgur.com/hAtA5cT.png)


#### 測試scullc模組

example.c
```cpp=
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/fcntl.h>

int main()
{
 int fd;
 char buf[]="this is a example for character devices driver by Liao712148!";//寫入scullc0裝置的內容

 char buf_read[4096];//scullc0裝置的內容讀入到該buf中

 
 if((fd=open("/dev/scullc0",O_RDWR))==-1)//使用者透過/dev中的裝置名稱（scullc0)開啟scullc0裝置

  printf("open scullc0 WRONG！\n");
 else
  printf("open scullc0 SUCCESS!\n");
  
 printf("buf is %s\n",buf);

 write(fd,buf,sizeof(buf));//把buf中的內容寫入scullc0裝置

 
 lseek(fd,0,SEEK_SET);//把檔案指標重新定位到檔案開始的位置

 
 read(fd,buf_read,sizeof(buf));//把scullc0裝置中的內容讀入到buf_read中

 
 printf("buf_read is %s\n",buf_read);
 
 return 0;
}

```

輸出結果：
![](https://i.imgur.com/Nm7n5kA.png)

#### 退出scullc模組

![](https://i.imgur.com/vM2sDQX.png)


2021 LDD3 第5章
===

contribute by `Liao712148`
[Liao712148](https://github.com/Liao712148/2021ldd)
###### tags: `Linux Device Drivers`, `Linux `

在持有lock時發生錯誤，==必須先釋放lock==，然後才將錯誤狀態碼傳回給使用者。

### 在scull裡使用lock
mutex的初始化，必須要發生在scull裝置可被系統其他部門取用之前。也就是mutex_init，必須要在scullc_setup_cdev之前。因為scullc_setup_cdev函式中會執行cdev_add()將字元裝置加入核心，也就是註冊字元裝置。當cdev_add()返回時，表示裝置已經生效了，核心隨時都有可能呼叫你的作業方法。
```cpp=
	for (i = 0; i < scullc_devs; i++) {
		scullc_devices[i].quantum = scullc_quantum;
		scullc_devices[i].qset = scullc_qset;
		mutex_init (&scullc_devices[i].lock);
		scullc_setup_cdev(scullc_devices + i, i);
	}
```

### Reader/Writer semphore
有時候只需要保護**讀取**或**改變**的動作即可。只要==共享資源的內容不被改變==，就可以被同時讀取。也就是說read-only的工作可以平行進行，而不必等待他readers離開critical section。

rwsem(reader/writer semaphore)定義於<linux/rwsem.h>中
down_read()可提供保護資源的read-only存取權，這類存取權沒有獨占性，可以同時授予多個不同的reader process，但是只要有任何一個proces仍然持有read-only存取權，就不會釋放任何write存取權。也就是說rwsem可容許一個writer或是多個readers同時擁有semaphore。==writer有優先權==（writer優先權高，所以只要有writer要semaphore，reader就強不到）當有任何writer試圖進入critical section，就沒有任何readers可獲准進入，直到所有writer都完成工作為止。因此reader可能會starvation。

### 完工通知

可能會使用semaphore來==同步==兩個工作，但是如果semaphore的佔用時間過常，或是搶semaphore的狀況很嚴重，就會嚴重的影響效率。就等待某活動完成工作此一用途而言，呼叫down()的後果幾乎總是等待，所以效能會受到嚴重損傷。
可以使用completion讓一個執行緒可將完工消息告訴另一個執行緒
*    等待完工通知的函式wait_for_completion()
    *    wait_for_completion是一個不可中斷的，表示若程式呼叫了它，但是沒有人送出完工通知，結果將造成一個殺不死的process。
*    送出完工通知的函式complete() / complete_all()

### spinlock

不同於semaphore/mutex，spinlock可用於==不得休眠的程式==，例如**interrupt_handlers**。

任何時侯只要spinlock被lock，相關==處理器的preemption就立刻失效==，因此使用spinlock時，持有spinlock的程式必須是atomic，==不能休眠==。也就是無論任何理由都不能釋出處理器，但是任何涉及到配置記憶體的操作，都有可能休眠（ex: kmalloc())因此要特別注意critical section中所呼叫的函式。
假設驅動程式順利的得到裝置的spinlock，因此可以對裝置進行存取，但是有可能在程式使用該裝置時觸發了也要使用該裝置的ISR，因此CPU會去執行ISR，但是ISR也需要spinlock才能對該裝置進行操作，但是沒有人可以去釋放proces所持有的spinloc，因此ISR得不到spinlock所以只好一直busy waiting，陷入==deadlock==。因此在持有spinlock的期間，會**暫時讓local CPU的interrupt 失效**。

如果必須要同時取得spinlock和mutex/semaphore的話，==必須先取得mutex/semaphore，再去取得spinlock==，因為在持有spinlock的情況之下，喝叫down()有可能造成休眠。

### 無鎖演算法
**單一消費者-單一生產者**
環狀暫存區（circular buffer)的資料結構，很適合用來應付無保護鎖的問題。網路卡驅動程式經常使用環狀暫存區來與處理器交換封包。
### atomic operation
將單一或是若干步驟視為最小操作單元，實際上就會遇到「要不就全部成功要不就全部失敗」的情況。

有以下兩種方法：
*    使用處理器的硬體指令(**缺乏移植性**)
*    使用linux 核心提供的atomic operation
    *    這些運算保證運算的過程中絕對可以一氣呵成，即使在多處理器系統上也不受影響。
    *    atmoic_t變數的連動性，==僅限於個別的連動函式==，也就是兩個atomic opertions組建起來的算式，不具有atomic的特性。

### seqlock
*    適用於小量、簡單、經常被存取、很少改的資源。
*    seqlock幾乎是放任readers可以自由存取受保護的資源，唯一要求是reader得必須自己檢查該次存取是否與writer"撞期"。
*    reader讀取受保護資料之前，必須先取得一個整數序號，然後才進入關鍵區。離開關鍵區時，則必須比較當時的序號與先前取得的序號是否相同，若不相同表示讀取的過程中，writer已經改變了資料，所以必須要再重新讀取。

