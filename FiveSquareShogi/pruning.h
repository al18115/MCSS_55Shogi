#pragma once
#include "node.h"
#include "tree.h"

class Pruning {
    //枝刈り
public:
    //指定されたノードから下を全て枝刈りする。返り値は刈ったノードの数
    size_t pruning(SearchNode* root, double pruning_border);
    //指定されたノードの探索深さ指標を基準とし、枝刈りを行う。
    static void pruningMass(SearchNode* node,double mass);
private:
    //指定されたノードに対して再帰的に枝刈りを行う
    size_t partialPruning(SearchNode* node, std::vector<SearchNode*>&history, double select = -1, int depth = 10, double backupRate = 1);
    //実際の枝刈り処理を行う
    static int pruningExecuter(SearchNode* node);
    bool isPruning(SearchNode* node, double select = 1, int depth = 10, double backupRate = 1);
    ////深さバックアップ温度再計算用の温度
    //double T_d = 100;
    double pruningBorder = 0.01;
    double pruningBorderEval = 50000;
    bool pruning_on = false;
    //枝刈りのタイプ。0は実現確率 1は深さ
    int pruning_type = 0;
    //枝刈りをしない深さ
    int pruning_depth = 5;
    double pruning_T_c = 40;
    //枝刈り時に残す残すノードの数(上位ノード)
    int leaveNodeCount = 0;

};
