An opensource anti-malware engine

detection:

  1. mp/fp scanner
    1. mp: idea from microsoft, ikraus, rising, ...
    1. fp: fix point match
  1. win32/64 emulator(based qemu)
    1. so big, i will do my best.
  1. malware definition data file
    1. diff
    1. patch
  1. heuristic
    1. pe/doc/tiff/pdf/rtf/...
    1. behaves-like
    1. looks-like
  1. packer recognition and unpacking mechanism

platforms:

  1. x86/64,MIPS
  1. windows,linux

Conceptual code：

#2014/1/17:

  1. VX64 CPU: completed, based QEMU 1.4, thanks fansj.
  1. KsVirtRAM: RAM + Unlimited PageFile, completed.
  1. KoSection: completed.
  1. KsPageTable: long mode, PAE, 4K Paging, completed.
  1. KsAddressSpace: completed. 64 bit virtual address.
  1. KsObjectTree: completed, supports SymbolLink(ReparsePoint)
  1. UmoView: View of Section, completed, MEM\_MAPPED.
  1. UmoPrivate: manage all private pages in process, alloced by user, created by CopyWrite mechanism, MEM\_PRIVATE.
  1. UmoImage: image section, completed, MEM\_IMAGE.
http://www.mcafee.com/us/resources/reports/rp-anti-malware-engines.pdf

http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.335.8246&rep=rep1&type=pdf

https://sparrow.ece.cmu.edu/group/731-s07/readings/amd2_24593.pdf

http://www.sophos.com/en-us/medialibrary/PDFs/technical%20papers/malicious_pdfs.pdf?dl=true

#2014/2/14:
  1. 丢失了一些代码，预计花一个月的时间把概念性代码转成DEMO代码，并且可以完成指令执行和GuestVM的访问，在host暴露一些基本的内存分配释放函数。
  1. 完成上面的事务后，开始进行：PE加载，Win32进程环境模拟
  1. FakeDLL的实现方式为：无stub代码模式，FakeDLL中仅原始文件的PE头和导入表（位置均不发生变化），然后采用自定义容器文件储存和访问文件原始内容。
  1. API的catch方式为：NX特性出发PF异常后分析RVA地址
  1. 目前已经有FakeDLL的生成代码，解决完"不存在文件快识别"的问题后，及可以正式使用


#2014/3/10
  1. 完成了基本的对象模型
  1. 完成了PEObject,Section,View,Image,File,Directory,Volume，VirtualFile，RegistryKey，RegistryValue等对象
  1. 重写了VMSPACE，PageTable，ObjectTree，物理内存
  1. PE加载大致流程：创建File，创建VirtualFile，创建PEObject，创建Image，分配VMSPACE，绑定Image，修重定位/导入表；
  1. 引入VirtualFile，实现Image的重定位和导入表修改时不影响文件内容，不需要直接修改VM；未来还将用于kernel的瞬间恢复，包括重新设计的ObjectTree实现，也是为快速还原整个kernel考虑的，再引入ObjectScope的概念，创建对象时，需要指定Scope，kernel|session|process，kernel范围的对象，kernel在就在；session范围的对象，kernel还原时将被清除；process范围的对象，process销毁时被清除。举例：一个进程创建的文件，被认为是session范围的，当进程退出后，不会消失（除非自删除属性），必须等到手工发起kernel还原操作才可以；kernel范围的对象，如果支持rollback接口，则会在kernel还原时被调用，例如，kernel本身内置的一些文件、注册表键值，就需要支持rollback接口，如果本次session中的进程修改了这些文件，还原时需要变回原来内容。
  1. VirtualFile实现了读缓存、写缓存、Windows读写，当VirtualFile形成栈时，接近栈顶的VirtualFile会关闭读缓存，因为下级VirtualFile有缓存，读是很快的，不需要再浪费内存的。
  1. PEObject支持了Data接口，暴露rva寻址的读写，内部转换成FileOffset对下级Data进行读写。
  1. Image则只暴露的segment的获取接口，目前只用到这一步,未来重构FakeDLL后，肯定需要再增加接口。
  1. 到今天为止，把物理内存和几个windows内存相关的对象都初步实现完了，简单的测试也通过了，包括不同的view映射同一个section，section可以正确处理Image和Data，VM释放时，自动释放View，物理内存失效时，view会积极相应修改页表。

#2014/3/16
  1. 本周主要完成了和QEMU的对接，点亮了Process的缺页处理

#2013/3/26
  1. 今天把PE加载全部搞完了，Windows7开始MiniWin模式的API导入的解决办法目前还没有找到，只能简单的通过文件名判断将api-ms-win-xxx中的API转接到对应的另外几个API中，后期估计得建个表来解决此问题。
  1. 接下来就是PEB/TEB的数据模拟了。
  1. QEMU在跑64位的Virus.Expiro时出现了异常，这个问题不好解决，暂时hold



I HAVE NO PLAN TO SUPPORT ARCHIVES NOW !