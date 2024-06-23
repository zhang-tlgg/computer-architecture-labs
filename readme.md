2024 THU 计算机系统结构实验作业：Tomasulo + Cache

都只完成了必做部分，在各自分支之下。

---

# Tomasulo 模拟器

2024 计算机系统结构 Tomasulo 模拟器实验框架

指令集使用 RISC-V 32 IM

实验文档在 [这里](https://lab.cs.tsinghua.edu.cn/ca-lab-docs/labs/tomasulo/) 发布，可以查阅实验要求和实验指导书。

有任何疑问，请在 **课程交流群 / 网络学堂答疑区** 联系助教。

## 目录结构

```
.
├── backend             # 后端文件
├── cache               # Cache 实现
├── cache-exp           # 缓存实验文件
├── checkfiles          # 测例文件
├── CMakeLists.txt
├── common              # 通用源码
├── frontend            # 前端文件
├── include             # 头文件
├── program             # 可执行程序
├── readme.md 
├── test                # 测试用户程序
└── thirdparty          # 第三方代码
```

## 注意事项

1. ICache 地址映射至 `0x80000000 ~ 0x803FFFFF`，DCache 地址映射至 `0x80400000 ~ 0x807FFFFF`
2. 已经初始化 sp 为 0x80800000，gp 为 0x80400000
3. 目前使用 `0x0000000b` 作为 exit mark

## 实验准备

实验之前，需要准备好如下工具：

1. cmake，g++: cmake 最低版本为 3.15，g++ 最低版本为 9.0.0

2. riscv 工具链：riscv-gnu-linux- 工具链。推荐使用 linux 环境，工具链可以通过 apt 直接安装。
   
   ```bash
   sudo apt install gcc-riscv64-unknown-elf # 注意是 gcc
   sudo apt install g++-riscv64-linux-gnu   # 选一个安装就行
   ```

## 使用方法

该实验框架使用 cmake 编译，编译方法如下：

```bash
mkdir build
cd build
cmake ..
make
```

如果使用 riscv64-linux-gnu-toolchain，则在编译前修改 `./test/CMakeLists.txt` 中的 Toolchain prefix 

`test` 文件夹用于存放测例，如果增加了新的测例，请手动重新 cmake。

之后，可以运行 checker 检查实现的正确性，如：

对 riscv64-unknown-elf-toolchain：

```bash
./checker -f ./test/selection_sort -c ../checkfiles/selection_sort.chk 
```

对 riscv64-linux-gnu-toolchain:

```bash
./checker -f ./test/selection_sort -c ../checkfiles/selection_sort_linux.chk
```

若最后一行显示：`[   OK    ] 16 testcase(s) passed`，则说明当前测例通过，可以继续测试其他测例。

所有需要实现 / 修改的地方已经使用 `TODO:` 标出，可以全局搜索进行定位。

请不要修改任何没有使用 `TODO:` 标记出的文件，这可能会导致你无法通过后续测试。

你可以自行修改 checkfile，但我们最后仍将在 `unknown-elf` 工具链，以及原版的 checkfile 下进行测试。

## 关于 Gitlab CI **（！重要！）**

正常情况下，当你执行 `git push`将本地仓库push到远端后，gitlab即会将你的评测任务加入到执行队列中。

为了保证 `Gitlab CI` 的正常运行，**请不要修改 `.gitlab-ci.yml` ，请不要修改 `.gitlab-ci.yml` ，请不要修改 `.gitlab-ci.yml` 。**

### CI 任务说明

只包含一个任务，进行 tomasulo 模拟器的测试。将会依次运行 `./check_tomasulo.sh` 和 `./check_predict.sh` 两个脚本，分别进行不带分支预测和带有分支预测的测试。

关于 checker 的运行逻辑和测试内容请参考实验文档和有关代码。

如果通过 CI，显示 Job Succeeded 即说明你通过了带有分支预测器的测试样例。
