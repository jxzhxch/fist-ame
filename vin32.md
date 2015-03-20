Windows32/64模拟

内核对象/地址空间
目的是完整模拟Windows地址空间管理机制。
地址空间有多个Region组成。起初就成只有一个空闲状态的Region，那就是0到最高用户地址。
随着虚拟地址的分配与释放，Region被不停的分裂与合并。
每个Region可以绑定一个UMO(UserSpace-Memory-Object)。
不过目前只有两种UMO，那就是View和Image。
所有Private类型的页，都被地址空间的PrivatePagesManager管理。
当View/Image中的页因为写拷贝特性进行了重新分配后，该页变为Private，添加到PrivatePagesManager，并受其管理。

AddressSpace
> region
> > block
> > block

> region
> > block

> region
> > block
> > block
> > block

> region
> > block
> > block
> > block
> > block

[KoSection](KoSection.md)
Section对象需要同一个后台文件绑定，例如：数据文件对象、映像文件对象等。
Section对象分配固定大小的物理内存页面槽来装载文件数据，使虚拟地址可以映射到对应的物理地址。
某个文件偏移的数据页，通过"(offset>>PAGE\_SIZE)/size"这样的公式计算获得此文件偏移对应的物理内存页放到何处。
这种情况发生时，后先看该位置是否已经存在了一分配的物理内存页，如果已经存在并且内容被改写了，那就需要将数据刷回到文件。接着释放这块物理地址（此时会触发失效回调，进而使页表失效）。接着分配一块物理内存，指该位置的文件数据读取到物理页中，并最终返回给调用者这个物理页对象；
这样一来，一个Section对象总是只保留固定数量的文件数据块在物理内存中，降低了物理内存的占用。
当用户创建Section时没有指定后台文件，则为其创建一个临时的后台文件，并且生命周期同Section对象。