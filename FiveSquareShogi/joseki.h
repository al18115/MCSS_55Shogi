﻿#pragma once
#include "node.h"
#include "kyokumen.h"
#include <iostream>
#include <fstream>
#include "usi.h"
#include <unordered_map>
#include "tree.h"
#include "josekioption.h"
#include "jdatabase.h"
#include "josekiinput.h"
#include "josekioutput.h"

class Joseki {
public:
    Joseki();

    //オプション設定
    JosekiOption option;

    void setOption(std::vector<std::string>tokens);
    void coutOption();

    void init(SearchTree* tree);
    void fin(std::vector<SearchNode*>history);
private:
    JosekiDataBase josekiDataBase;
    JosekiInput input;
    JosekiOutput output;

};