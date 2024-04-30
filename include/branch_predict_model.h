// 如果有需要，该文件可以任意进行修改
struct BpuUpdateData {
    unsigned pc;
    bool isCall, isReturn, isBranch, branchTaken;
    unsigned jumpTarget;
};

struct BranchPredictBundle {
    bool predictJump = false;
    unsigned predictTarget = 0;
};
