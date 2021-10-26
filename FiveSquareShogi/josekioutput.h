﻿#pragma once
#include "tree.h"
#include "josekinode.h"
#include "josekioption.h"
#include "pruning.h"

class JosekiOutput {
    //出力
public:
    JosekiOutput();
    //定跡書き出し
    void josekiOutput(const std::vector<SearchNode*> const history);
    //末端からのバックアップ
    void backUp(std::vector<SearchNode*> history);
    JosekiOption option;
private:
    //情報ファイルへの書き出し
    bool outputInfo(const std::vector<SearchNode*> const history);
    //枝刈り
    Pruning pruning;
};
